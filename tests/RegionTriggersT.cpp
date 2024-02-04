// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "TestHelpers.h"
#include "sfizz/Synth.h"
#include "sfizz/Region.h"
#include "sfizz/Layer.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;
using namespace sfz;

TEST_CASE("Basic triggers", "Region triggers")
{
    MidiState midiState;
    Region region { 0 };

    region.parseOpcode({ "sample", "*sine" });
    SECTION("key")
    {
        region.parseOpcode({ "key", "40" });
        Layer layer { region, midiState };
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!layer.registerCC(63, 64_norm, 0.0f));
    }
    SECTION("lokey and hikey")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "42" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerNoteOn(39, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOn(42, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(43, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(42, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(42, 64_norm, 0.5f));
        REQUIRE(!layer.registerCC(63, 64_norm, 0.0f));
    }
    SECTION("key and release trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(41, 64_norm, 0.5f));
        REQUIRE(!layer.registerCC(63, 64_norm, 0.0f));
    }
    SECTION("key and release_key trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release_key" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!layer.registerNoteOff(41, 64_norm, 0.5f));
        REQUIRE(!layer.registerCC(63, 64_norm, 0.0f));
    }
    // TODO: first and legato triggers
    SECTION("lovel and hivel")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lovel", "60" });
        region.parseOpcode({ "hivel", "70" });
        Layer layer { region, midiState };
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(layer.registerNoteOn(40, 60_norm, 0.5f));
        REQUIRE(layer.registerNoteOn(40, 70_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(41, 71_norm, 0.5f));
        REQUIRE(!layer.registerNoteOn(41, 59_norm, 0.5f));
    }

    SECTION("lorand and hirand")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.34f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.35f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.36f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.37f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.38f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.39f));
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.40f));
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.41f));
    }

    SECTION("lorand and hirand on 1.0f")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.34f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.35f));
        REQUIRE(layer.registerNoteOn(40, 64_norm, 1.0f));
    }

    SECTION("Disable key trigger")
    {
        region.parseOpcode({ "key", "40" });
        Layer layer1 { region, midiState };
        REQUIRE(layer1.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "hikey", "-1" });
        Layer layer2 { region, midiState };
        REQUIRE(!layer2.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "hikey", "40" });
        Layer layer3 { region, midiState };
        REQUIRE(layer3.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "key", "-1" });
        Layer layer4 { region, midiState };
        REQUIRE(!layer4.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "key", "40" });
        Layer layer5 { region, midiState };
        REQUIRE(layer5.registerNoteOn(40, 64_norm, 1.0f));
    }

    SECTION("on_loccN, on_hiccN")
    {
        region.parseOpcode({ "on_locc47", "64" });
        region.parseOpcode({ "on_hicc47", "68" });
        Layer layer1 { region, midiState };
        REQUIRE(!layer1.registerCC(47, 63_norm, 0.0f));
        REQUIRE(layer1.registerCC(47, 64_norm, 0.0f));
        REQUIRE(layer1.registerCC(47, 65_norm, 0.0f));
        region.parseOpcode({ "hikey", "-1" });
        Layer layer2 { region, midiState };
        REQUIRE(layer2.registerCC(47, 64_norm, 0.0f));
        REQUIRE(layer2.registerCC(47, 65_norm, 0.0f));
        REQUIRE(layer2.registerCC(47, 66_norm, 0.0f));
        REQUIRE(layer2.registerCC(47, 67_norm, 0.0f));
        REQUIRE(layer2.registerCC(47, 68_norm, 0.0f));
        REQUIRE(!layer2.registerCC(47, 69_norm, 0.0f));
        REQUIRE(!layer2.registerCC(40, 64_norm, 0.0f));
    }

    SECTION("lorand and hirand with CC triggers")
    {
        region.parseOpcode({ "on_locc47", "64" });
        region.parseOpcode({ "on_hicc47", "68" });
        region.parseOpcode({ "hikey", "-1" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerCC(47, 64_norm, 0.0f));
        REQUIRE(!layer.registerCC(47, 64_norm, 0.34f));
        REQUIRE(layer.registerCC(47, 64_norm, 0.35f));
        REQUIRE(layer.registerCC(47, 64_norm, 0.36f));
        REQUIRE(layer.registerCC(47, 64_norm, 0.37f));
        REQUIRE(layer.registerCC(47, 64_norm, 0.38f));
        REQUIRE(layer.registerCC(47, 64_norm, 0.39f));
        REQUIRE(!layer.registerCC(47, 64_norm, 0.40f));
    }

    SECTION("on_loccN does not disable key triggering")
    {
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "on_locc1", "127" });
        region.parseOpcode({ "on_hicc1", "127" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerCC(1, 126_norm, 0.0f));
        REQUIRE(!layer.registerCC(2, 127_norm, 0.0f));
        REQUIRE(layer.registerCC(1, 127_norm, 0.0f));
        REQUIRE(layer.registerNoteOn(64, 127_norm, 0.5f));
    }

    SECTION("on_loccN does not disable key triggering, but adding key=-1 does")
    {
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "on_locc1", "127" });
        region.parseOpcode({ "on_hicc1", "127" });
        region.parseOpcode({ "key", "-1" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerCC(1, 126_norm, 0.0f));
        REQUIRE(layer.registerCC(1, 127_norm, 0.0f));
        REQUIRE(!layer.registerNoteOn(64, 127_norm, 0.5f));
    }

    SECTION("on_loccN does not disable key triggering, but adding hikey=-1 does")
    {
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "on_locc1", "127" });
        region.parseOpcode({ "on_hicc1", "127" });
        region.parseOpcode({ "hikey", "-1" });
        Layer layer { region, midiState };
        REQUIRE(!layer.registerCC(1, 126_norm, 0.0f));
        REQUIRE(!layer.registerCC(2, 127_norm, 0.0f));
        REQUIRE(layer.registerCC(1, 127_norm, 0.0f));
        REQUIRE(!layer.registerNoteOn(64, 127_norm, 0.5f));
    }
}

TEST_CASE("Legato triggers", "Region triggers")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    SECTION("First note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "first" });
        Layer layer { region, midiState };
        midiState.noteOnEvent(0, 40, 64_norm);
        REQUIRE(layer.registerNoteOn(40, 64_norm, 0.5f));
        midiState.noteOnEvent(0, 41, 64_norm);
        REQUIRE(!layer.registerNoteOn(41, 64_norm, 0.5f));
        midiState.noteOffEvent(0, 40, 0_norm);
        layer.registerNoteOff(40, 0_norm, 0.5f);
        midiState.noteOffEvent(0, 41, 0_norm);
        layer.registerNoteOff(41, 0_norm, 0.5f);
        midiState.noteOnEvent(0, 42, 64_norm);
        REQUIRE(layer.registerNoteOn(42, 64_norm, 0.5f));
    }

    SECTION("Second note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "legato" });
        Layer layer { region, midiState };
        midiState.noteOnEvent(0, 40, 64_norm);
        REQUIRE(!layer.registerNoteOn(40, 64_norm, 0.5f));
        midiState.noteOnEvent(0, 41, 64_norm);
        REQUIRE(layer.registerNoteOn(41, 64_norm, 0.5f));
        midiState.noteOffEvent(0, 40, 64_norm);
        layer.registerNoteOff(40, 0_norm, 0.5f);
        midiState.noteOffEvent(0, 41, 64_norm);
        layer.registerNoteOff(41, 0_norm, 0.5f);
        midiState.noteOnEvent(0, 42, 64_norm);
        REQUIRE(!layer.registerNoteOn(42, 64_norm, 0.5f));
    }
}

TEST_CASE("[Triggers] sw_vel, basic")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=60 sample=kick.wav
        <region> key=62 sw_previous=60 sw_vel=previous sample=snare.wav
    )");
    synth.noteOn(0, 60, 127);
    synth.noteOn(10, 62, 10);
    synth.renderBlock(buffer);
    synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
    std::vector<std::string> expected {
        "/num_active_voices,i : { 2 }",
        "/voice0/trigger_value,f : { 1 }",
        "/voice1/trigger_value,f : { 1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Triggers] sw_vel, without sw_previous")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=60 sample=kick.wav
        <region> key=62 sw_vel=previous sample=snare.wav
    )");
    synth.noteOn(0, 60, 127);
    synth.noteOn(10, 62, 10);
    synth.renderBlock(buffer);
    synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
    std::vector<std::string> expected {
        "/num_active_voices,i : { 2 }",
        "/voice0/trigger_value,f : { 1 }",
        "/voice1/trigger_value,f : { 1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Triggers] sw_vel, with a note in between")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=60 sample=kick.wav
        <region> key=62 sw_vel=previous sample=snare.wav
        <region> key=64 sample=closedhat.wav
    )");
    synth.noteOn(0, 60, 127);
    synth.noteOn(5, 64, 63);
    synth.noteOn(10, 62, 10);
    synth.renderBlock(buffer);
    synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice2/trigger_value", "", nullptr);
    std::vector<std::string> expected {
        "/num_active_voices,i : { 3 }",
        "/voice0/trigger_value,f : { 1 }",
        "/voice1/trigger_value,f : { 0.496063 }",
        "/voice2/trigger_value,f : { 0.496063 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Triggers] sw_vel, with a note in between and sw_previous")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=60 sample=kick.wav
        <region> key=62 sw_previous=60 sw_vel=previous sample=snare.wav
        <region> key=64 sample=closedhat.wav
    )");
    synth.noteOn(0, 60, 127);
    synth.noteOn(5, 64, 63);
    synth.noteOn(10, 62, 10);
    synth.renderBlock(buffer);
    synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
    synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
    std::vector<std::string> expected {
        "/num_active_voices,i : { 2 }",
        "/voice0/trigger_value,f : { 1 }",
        "/voice1/trigger_value,f : { 0.496063 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Triggers] sw_vel, consider the previous velocity for triggers")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=60 sample=kick.wav
        <region> key=62 sw_previous=60 sw_vel=previous sample=snare.wav lovel=63
    )");

    SECTION("Should trigger") {
        synth.noteOn(0, 60, 127);
        synth.noteOn(10, 62, 10);
        synth.renderBlock(buffer);
        synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
        std::vector<std::string> expected {
            "/num_active_voices,i : { 2 }",
            "/voice0/trigger_value,f : { 1 }",
            "/voice1/trigger_value,f : { 1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Should not trigger") {
        synth.noteOn(0, 60, 10);
        synth.noteOn(10, 62, 127);
        synth.renderBlock(buffer);
        synth.dispatchMessage(client, 0, "/num_active_voices", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/trigger_value", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/trigger_value", "", nullptr);
        std::vector<std::string> expected {
            "/num_active_voices,i : { 1 }",
            "/voice0/trigger_value,f : { 0.0787402 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Triggers] Honor lorand/hirand on CC triggers")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> sample=*sine hikey=-1 start_locc64=63 start_hicc64=127 lorand=0 hirand=0.5
        <region> sample=*saw hikey=-1 start_locc64=63 start_hicc64=127 lorand=0.5 hirand=1
    )");

    synth.hdcc(0, 64, 10_norm);
    REQUIRE(numPlayingVoices(synth) == 0);
    synth.hdcc(0, 64, 100_norm);
    REQUIRE(numPlayingVoices(synth) == 1);
}

TEST_CASE("[Triggers] Offed voices with CC triggers do not activate release triggers")
{
    Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> sample=*sine hikey=-1 start_locc64=63 start_hicc64=127 group=1 off_by=2
        <region> sample=*saw hikey=-1 start_locc64=0 start_hicc64=62 group=2
        <region> sample=*noise trigger=release_key
    )");

    synth.hdcc(0, 64, 100_norm);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "*sine" });
    synth.hdcc(10, 64, 10_norm);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "*saw" });
}

TEST_CASE("[Triggers] lotimer")
{
    Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer { 2, 480 };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> key=40 lotimer=0.1 sample=kick.wav loop_mode=one_shot
    )");

    // Advance time, t = 0.2s
    for (int i = 0; i < 20; ++i)
        synth.renderBlock(buffer);

    synth.noteOn(0, 40, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });

    // Advance time by 0.02s and send a note off
    synth.renderBlock(buffer);
    synth.noteOff(0, 40, 100);
    synth.renderBlock(buffer);

    // t = 0.22s, not allowed to retrigger
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });
    synth.noteOn(0, 40, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });

    // Advance time by 0.08s and send a note off
    synth.renderBlock(buffer);
    synth.noteOff(0, 40, 100);
    for (int i = 0; i < 7; ++i)
        synth.renderBlock(buffer);

    // t = 0.3s, retriggering OK
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });
    synth.noteOn(0, 40, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav", "kick.wav" });
}

TEST_CASE("[Triggers] lotimer on CC")
{
    Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer { 2, 480 };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> start_locc4=100 start_hicc4=100 lotimer=0.1 sample=kick.wav loop_mode=one_shot
    )");

    // Advance time, t = 0.2s
    for (int i = 0; i < 20; ++i)
        synth.renderBlock(buffer);

    REQUIRE(playingSamples(synth) == std::vector<std::string> { });
    synth.hdcc(0, 4, 100_norm);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });

    // Advance time by 0.02s and reset CC
    synth.renderBlock(buffer);
    synth.hdcc(0, 4, 0);
    synth.renderBlock(buffer);

    // t = 0.22s, not allowed to retrigger
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });
    synth.hdcc(0, 4, 100_norm);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });

    // Advance time by 0.08s and reset CC
    synth.renderBlock(buffer);
    synth.hdcc(0, 4, 0);
    for (int i = 0; i < 7; ++i)
        synth.renderBlock(buffer);

    // t = 0.3s, retriggering OK
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav" });
    synth.hdcc(0, 4, 100_norm);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "kick.wav", "kick.wav" });
}

TEST_CASE("[Triggers] hitimer (with group)")
{
    Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer { 2, 480 };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_vel.sfz", R"(
        <region> group=1 key=40 sample=snare.wav loop_mode=one_shot
        <region> group=1 key=41 hitimer=0.1 sample=kick.wav loop_mode=one_shot
    )");

    // Advance time to 0.1s
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);

    // Not triggering before snare
    synth.noteOn(0, 41, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { });
    synth.noteOn(0, 40, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav" });

    // OK to trigger now
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav" });
    synth.noteOn(0, 41, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav", "kick.wav" });

    // Advance time
    for (int i = 0; i < 5; ++i)
        synth.renderBlock(buffer);

    // t = 0.15s, still OK to trigger now
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav", "kick.wav" });
    synth.noteOn(0, 41, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav", "kick.wav", "kick.wav" });

    // Advance time (it has to be 0.1s because playing the kick resets the timer)
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);
    
    // t = 0.25s, not triggering a new kick 
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav", "kick.wav", "kick.wav" });
    synth.noteOn(0, 41, 100);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "snare.wav", "kick.wav", "kick.wav" });
}

TEST_CASE("[Triggers] Respect poly-aftertouch note values when triggering on cc130")
{
    Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/poly_at_trigger.sfz", R"(
        <region> key=55 on_locc130=127 on_hicc130=127 sample=*saw
        <region> key=57 on_locc130=127 on_hicc130=127 sample=*sine
    )");

    synth.polyAftertouch(0, 54, 127);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { });
    synth.polyAftertouch(0, 56, 127);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { });
    synth.polyAftertouch(0, 58, 127);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { });
    synth.polyAftertouch(0, 55, 127);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "*saw" });
    synth.polyAftertouch(10, 57, 127);
    REQUIRE(playingSamples(synth) == std::vector<std::string> { "*saw", "*sine" });
}
