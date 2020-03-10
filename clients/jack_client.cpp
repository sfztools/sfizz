// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "sfizz/Synth.h"
#include <absl/flags/parse.h>
#include <absl/flags/flag.h>
#include <absl/types/span.h>
#include <atomic>
#include <cstddef>
#include <ios>
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/types.h>
#include <ostream>
#include <signal.h>
#include <string_view>
#include <chrono>
#include <thread>

static jack_port_t* midiInputPort;
static jack_port_t* outputPort1;
static jack_port_t* outputPort2;
static jack_client_t* client;

namespace midi {
constexpr uint8_t statusMask { 0b11110000 };
constexpr uint8_t channelMask { 0b00001111 };
constexpr uint8_t noteOff { 0x80 };
constexpr uint8_t noteOn { 0x90 };
constexpr uint8_t polyphonicPressure { 0xA0 };
constexpr uint8_t controlChange { 0xB0 };
constexpr uint8_t programChange { 0xC0 };
constexpr uint8_t channelPressure { 0xD0 };
constexpr uint8_t pitchBend { 0xE0 };
constexpr uint8_t systemMessage { 0xF0 };

constexpr uint8_t status(uint8_t midiStatusByte)
{
    return midiStatusByte & statusMask;
}
constexpr uint8_t channel(uint8_t midiStatusByte)
{
    return midiStatusByte & channelMask;
}

constexpr int buildAndCenterPitch(uint8_t firstByte, uint8_t secondByte)
{
    return (int)(((unsigned int)secondByte << 7) + (unsigned int)firstByte) - 8192;
}
}

static std::atomic<bool> keepRunning [[maybe_unused]] { true };

int process(jack_nframes_t numFrames, void* arg [[maybe_unused]])
{
    auto synth = reinterpret_cast<sfz::Synth*>(arg);

    auto buffer = jack_port_get_buffer(midiInputPort, numFrames);
    assert(buffer);

    auto numMidiEvents = jack_midi_get_event_count(buffer);
    jack_midi_event_t event;

    // Midi dispatching
    for (uint32_t i = 0; i < numMidiEvents; ++i) {
        if (jack_midi_event_get(&event, buffer, i) != 0)
            continue;

        if (event.size == 0)
            continue;

        switch (midi::status(event.buffer[0])) {
        case midi::noteOff:
            // DBG("[MIDI] Note " << +event.buffer[1] << " OFF at time " << event.time);
            synth->noteOff(event.time, event.buffer[1], event.buffer[2]);
            break;
        case midi::noteOn:
            // DBG("[MIDI] Note " << +event.buffer[1] << " ON at time " << event.time);
            synth->noteOn(event.time, event.buffer[1], event.buffer[2]);
            break;
        case midi::polyphonicPressure:
            // DBG("[MIDI] Polyphonic pressure on at time " << event.time);
            break;
        case midi::controlChange:
            // DBG("[MIDI] CC " << +event.buffer[1] << " at time " << event.time);
            synth->cc(event.time, event.buffer[1], event.buffer[2]);
            break;
        case midi::programChange:
            // DBG("[MIDI] Program change at time " << event.time);
            break;
        case midi::channelPressure:
            // DBG("[MIDI] Channel pressure at time " << event.time);
            break;
        case midi::pitchBend:
            synth->pitchWheel(event.time, midi::buildAndCenterPitch(event.buffer[1], event.buffer[2]));
            // DBG("[MIDI] Pitch bend at time " << event.time);
            break;
        case midi::systemMessage:
            // DBG("[MIDI] System message at time " << event.time);
            break;
        }
    }

    auto leftOutput = reinterpret_cast<float*>(jack_port_get_buffer(outputPort1, numFrames));
    auto rightOutput = reinterpret_cast<float*>(jack_port_get_buffer(outputPort2, numFrames));
    synth->renderBlock({ { leftOutput, rightOutput }, numFrames });

    return 0;
}

int sampleBlockChanged(jack_nframes_t nframes, void* arg [[maybe_unused]])
{
    if (arg == nullptr)
        return 0;

    auto synth = reinterpret_cast<sfz::Synth*>(arg);
    // DBG("Sample per block changed to " << nframes);
    synth->setSamplesPerBlock(nframes);
    return 0;
}

int sampleRateChanged(jack_nframes_t nframes, void* arg [[maybe_unused]])
{
    if (arg == nullptr)
        return 0;

    auto synth = reinterpret_cast<sfz::Synth*>(arg);
    // DBG("Sample rate changed to " << nframes);
    synth->setSampleRate(nframes);
    return 0;
}

static bool shouldClose { false };

static void done(int sig [[maybe_unused]])
{
    std::cout << "Signal received" << '\n';
    shouldClose = true;
    // if (client != nullptr)

    // exit(0);
}

ABSL_FLAG(std::string, client_name, "sfizz", "Jack client name");
ABSL_FLAG(std::string, oversampling, "1x", "Internal oversampling factor (value values are x1, x2, x4, x8)");
ABSL_FLAG(uint32_t, preload_size, 8192, "Preloaded value");

int main(int argc, char** argv)
{
    // std::ios::sync_with_stdio(false);
    auto arguments = absl::ParseCommandLine(argc, argv);
    if (arguments.size() < 2) {
        std::cout << "You need to specify an SFZ file to load." << '\n';
        return -1;
    }

    auto filesToParse = absl::MakeConstSpan(arguments).subspan(1);
    const std::string clientName = absl::GetFlag(FLAGS_client_name);
    const std::string oversampling = absl::GetFlag(FLAGS_oversampling);
    const uint32_t preload_size = absl::GetFlag(FLAGS_preload_size);

    std::cout << "Flags" << '\n';
    std::cout << "- Client name: " << clientName << '\n';
    std::cout << "- Oversampling: " << oversampling << '\n';
    std::cout << "- Preloaded Size: " << preload_size << '\n';
    const auto factor = [&]() {
        if (oversampling == "x1") return sfz::Oversampling::x1;
        if (oversampling == "x2") return sfz::Oversampling::x2;
        if (oversampling == "x4") return sfz::Oversampling::x4;
        if (oversampling == "x8") return sfz::Oversampling::x8;
        return sfz::Oversampling::x1;
    }();

    std::cout << "Positional arguments:";
    for (auto& file : filesToParse)
        std::cout << " " << file << ',';
    std::cout << '\n';

    sfz::Synth synth;
    synth.setOversamplingFactor(factor);
    synth.setPreloadSize(preload_size);
    synth.loadSfzFile(filesToParse[0]);
    std::cout << "==========" << '\n';
    std::cout << "Total:" << '\n';
    std::cout << "\tMasters: " << synth.getNumMasters() << '\n';
    std::cout << "\tGroups: " << synth.getNumGroups() << '\n';
    std::cout << "\tRegions: " << synth.getNumRegions() << '\n';
    std::cout << "\tCurves: " << synth.getNumCurves() << '\n';
    std::cout << "\tPreloadedSamples: " << synth.getNumPreloadedSamples() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Included files:" << '\n';
    for (auto& file : synth.getIncludedFiles())
        std::cout << '\t' << file.string() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Defines:" << '\n';
    for (auto& define : synth.getDefines())
        std::cout << '\t' << define.first << '=' << define.second << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Unknown opcodes:";
    for (auto& opcode : synth.getUnknownOpcodes())
        std::cout << opcode << ',';
    std::cout << '\n';
    // std::cout << std::flush;

    jack_status_t status;
    client = jack_client_open(clientName.c_str(), JackNullOption, &status);
    if (client == nullptr) {
        std::cerr << "Could not open JACK client" << '\n';
        // if (status & JackFailure)
        //     std::cerr << "JackFailure: Overall operation failed" << '\n';
        return 1;
    }

    if (status & JackNameNotUnique) {
        std::cout << "Name was taken: assigned " << jack_get_client_name(client) << "instead" << '\n';
    }
    if (status & JackServerStarted) {
        std::cout << "Connected to JACK" << '\n';
    }

    synth.setSamplesPerBlock(jack_get_buffer_size(client));
    synth.setSampleRate(jack_get_sample_rate(client));

    jack_set_sample_rate_callback(client, sampleRateChanged, &synth);
    jack_set_buffer_size_callback(client, sampleBlockChanged, &synth);
    jack_set_process_callback(client, process, &synth);

    midiInputPort = jack_port_register(client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    if (midiInputPort == nullptr) {
        std::cerr << "Could not open MIDI input port" << '\n';
        return 1;
    }

    outputPort1 = jack_port_register(client, "output_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    outputPort2 = jack_port_register(client, "output_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (outputPort1 == nullptr || outputPort2 == nullptr) {
        std::cerr << "Could not open output ports" << '\n';
        return 1;
    }

    if (jack_activate(client) != 0) {
        std::cerr << "Could not activate client" << '\n';
        return 1;
    }

    auto systemPorts = jack_get_ports(client, nullptr, nullptr, JackPortIsPhysical | JackPortIsInput);
    if (systemPorts == nullptr) {
        std::cerr << "No physical output ports found" << '\n';
        return 1;
    }

    if (jack_connect(client, jack_port_name(outputPort1), systemPorts[0])) {
        std::cerr << "Cannot connect to physical output ports (0)" << '\n';
    }

    if (jack_connect(client, jack_port_name(outputPort2), systemPorts[1])) {
        std::cerr << "Cannot connect to physical output ports (1)" << '\n';
    }
    jack_free(systemPorts);

    signal(SIGHUP, done);
    signal(SIGINT, done);
    signal(SIGTERM, done);
    signal(SIGQUIT, done);

    while (!shouldClose){
#ifndef NDEBUG
        std::cout << "Allocated buffers: " << synth.getAllocatedBuffers() << '\n';
        std::cout << "Total size: " << synth.getAllocatedBytes()  << '\n';
#endif
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "Closing..." << '\n';
    jack_client_close(client);
    return 0;
}
