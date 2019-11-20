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

#pragma once
#include "FilePool.h"
#include "Parser.h"
#include "Region.h"
#include "LeakDetector.h"
#include "MidiState.h"
#include "AudioSpan.h"
#include "absl/types/span.h"
#include <absl/types/optional.h>
#include <random>
#include <set>
#include <string_view>
#include <vector>

namespace sfz {

class Synth : public Parser {
public:
    Synth();
    Synth(int numVoices);
    bool loadSfzFile(const fs::path& file) final;
    int getNumRegions() const noexcept;
    int getNumGroups() const noexcept;
    int getNumMasters() const noexcept;
    int getNumCurves() const noexcept;
    const Region* getRegionView(int idx) const noexcept;
    std::set<absl::string_view> getUnknownOpcodes() const noexcept;
    size_t getNumPreloadedSamples() const noexcept;

    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    void setSampleRate(float sampleRate) noexcept;
    float getVolume() const noexcept;
    void setVolume(float volume) noexcept; 

    void renderBlock(AudioSpan<float> buffer) noexcept;
    void noteOn(int delay, int channel, int noteNumber, uint8_t velocity) noexcept;
    void noteOff(int delay, int channel, int noteNumber, uint8_t velocity) noexcept;
    void cc(int delay, int channel, int ccNumber, uint8_t ccValue) noexcept;
    void pitchWheel(int delay, int channel, int pitch) noexcept;
    void aftertouch(int delay, int channel, uint8_t aftertouch) noexcept;
    void tempo(int delay, float secondsPerQuarter) noexcept;

    int getNumActiveVoices() const noexcept;
    int getNumVoices() const noexcept;
    void setNumVoices(int numVoices) noexcept;
    void garbageCollect() noexcept;
protected:
    void callback(absl::string_view header, const std::vector<Opcode>& members) final;

private:
    bool hasGlobal { false };
    bool hasControl { false };
    int numGroups { 0 };
    int numMasters { 0 };
    int numCurves { 0 };
    void clear();
    void resetVoices(int numVoices);
    void handleGlobalOpcodes(const std::vector<Opcode>& members);
    void handleControlOpcodes(const std::vector<Opcode>& members);
    void buildRegion(const std::vector<Opcode>& regionOpcodes);
    
    std::vector<Opcode> globalOpcodes;
    std::vector<Opcode> masterOpcodes;
    std::vector<Opcode> groupOpcodes;

    FilePool filePool;
    MidiState midiState;
    Voice* findFreeVoice() noexcept;
    std::vector<CCNamePair> ccNames;
    absl::optional<uint8_t> defaultSwitch;
    std::set<absl::string_view> unknownOpcodes;
    using RegionPtrVector = std::vector<Region*>;
    using VoicePtrVector = std::vector<Voice*>;
    std::vector<std::unique_ptr<Region>> regions;
    std::vector<std::unique_ptr<Voice>> voices;
    VoicePtrVector voiceViewArray;
    std::array<RegionPtrVector, 128> noteActivationLists;
    std::array<RegionPtrVector, 128> ccActivationLists;

    AudioBuffer<float> tempBuffer { 2, config::defaultSamplesPerBlock };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    float sampleRate { config::defaultSampleRate };
    float volume { Default::volume };

    std::uniform_real_distribution<float> randNoteDistribution { 0, 1 };
    unsigned fileTicket { 1 };

    std::atomic<bool> canEnterCallback { true };
    std::atomic<bool> inCallback { false };

    int numVoices { config::numVoices };

    LEAK_DETECTOR(Synth);
};

}