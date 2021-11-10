// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @brief This file holds some of the specific MidiState tests. Some tests on the
 * effects of the midi state are also available in e.g. RegionValueComputationT.cpp
 * and SynthT.cpp.
 */

#include "sfizz/MidiState.h"
#include "sfizz/Synth.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
#include "absl/strings/string_view.h"
#include "TestHelpers.h"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("[MidiState] Initial values")
{
    sfz::MidiState state;
    for (unsigned cc = 0; cc < sfz::config::numCCs; cc++)
        REQUIRE(state.getCCValue(cc) == 0_norm);
    REQUIRE( state.getPitchBend() == 0 );
}

TEST_CASE("[MidiState] Set and get CCs")
{
    sfz::MidiState state;
    state.ccEvent(0, 24, 23_norm);
    state.ccEvent(0, 123, 124_norm);
    REQUIRE(state.getCCValue(24) == 23_norm);
    REQUIRE(state.getCCValue(123) == 124_norm);
}

TEST_CASE("[MidiState] Set and get pitch bends")
{
    sfz::MidiState state;
    state.pitchBendEvent(0, 0.5f);
    REQUIRE(state.getPitchBend() == 0.5f);
    state.pitchBendEvent(0, 0.0f);
    REQUIRE(state.getPitchBend() == 0.0f);
}

TEST_CASE("[MidiState] Resetting things")
{
    sfz::MidiState state;
    state.pitchBendEvent(0, 0.7f);
    state.noteOnEvent(0, 64, 24_norm);
    state.ccEvent(0, 123, 124_norm);
    state.channelAftertouchEvent(0, 56_norm);
    state.polyAftertouchEvent(0, 64, 43_norm);
    state.advanceTime(1024);

    // Only reset note stuff
    state.resetNoteStates();
    REQUIRE(state.getNoteVelocity(64) == 0_norm);
    REQUIRE(state.getNoteDuration(64) == 0_norm);
    REQUIRE(state.getActiveNotes() == 0);

    // Extended CCs too
    REQUIRE(state.getCCValue(131) == 0.0f);
    REQUIRE(state.getCCValue(132) == 0.0f);
    REQUIRE(state.getCCValue(133) == 0.0f);
    REQUIRE(state.getCCValue(134) == 0.0f);
    REQUIRE(state.getCCValue(135) == 0.0f);
    REQUIRE(state.getCCValue(136) == 0.0f);
    REQUIRE(state.getCCValue(137) == 0.0f);

    // State isn't reset
    REQUIRE(state.getPitchBend() != 0.0f);
    REQUIRE(state.getCCValue(123) != 0_norm);
    REQUIRE(state.getChannelAftertouch() != 0_norm);
    REQUIRE(state.getPolyAftertouch(64) != 0_norm);

    state.resetEventStates(); // But now it is
    REQUIRE(state.getPitchBend() == 0.0f);
    REQUIRE(state.getCCValue(123) == 0_norm);
    REQUIRE(state.getChannelAftertouch() == 0_norm);
    REQUIRE(state.getPolyAftertouch(64) == 0_norm);
}

TEST_CASE("[MidiState] Flushing state")
{
    sfz::MidiState state;
    state.pitchBendEvent(40, 0.7f);
    state.ccEvent(100, 123, 124_norm);
    state.channelAftertouchEvent(20, 56_norm);
    state.polyAftertouchEvent(80, 64, 43_norm);

    REQUIRE(state.getCCEvents(123).size() > 1);
    REQUIRE(state.getChannelAftertouchEvents().size() > 1);
    REQUIRE(state.getPolyAftertouchEvents(64).size() > 1);
    REQUIRE(state.getPitchEvents().size() > 1);

    state.flushEvents();
    REQUIRE(state.getCCEvents(123).size() == 1);
    REQUIRE(state.getChannelAftertouchEvents().size() == 1);
    REQUIRE(state.getPolyAftertouchEvents(64).size() == 1);
    REQUIRE(state.getPitchEvents().size() == 1);

    REQUIRE(state.getCCValue(123) == 124_norm);
    REQUIRE(state.getChannelAftertouch() == 56_norm);
    REQUIRE(state.getPolyAftertouch(64) == 43_norm);
    REQUIRE(state.getPitchBend() == 0.7f);
}

TEST_CASE("[MidiState] Set and get note velocities")
{
    sfz::MidiState state;
    state.noteOnEvent(0, 64, 24_norm);
    REQUIRE(+state.getNoteVelocity(64) == 24_norm);
    state.noteOnEvent(0, 64, 123_norm);
    REQUIRE(+state.getNoteVelocity(64) == 123_norm);
}

TEST_CASE("[MidiState] Extended CCs")
{
    sfz::MidiState state;
    state.ccEvent(0, 142, 64_norm); // should not trap
}

TEST_CASE("[MidiState] Last note velocity")
{
    sfz::MidiState state;
    state.noteOnEvent(0, 62, 64_norm);
    state.noteOnEvent(0, 60, 10_norm);
    REQUIRE(state.getVelocityOverride() == 64_norm);
}


TEST_CASE("[CC] Extended CCs on offset and delay")
{
    sfz::Synth synth;
    std::vector<std::string> messageList;
    sfz::Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setSampleRate(48000);

    SECTION("CC131 - Note on velocity")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> key=60 delay_cc131=1 sample=kick.wav
            <region> key=61 offset_cc131=100 sample=snare.wav
        )");
        synth.hdNoteOn(0, 60, 0.0f);
        synth.hdNoteOn(0, 60, 0.5f);
        synth.hdNoteOn(0, 61, 0.0f);
        synth.hdNoteOn(0, 61, 0.5f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice3/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 0 }",
            "/voice1/remaining_delay,i : { 24000 }",
            "/voice2/source_position,i : { 0 }",
            "/voice3/source_position,i : { 50 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC132 - Note off velocity")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> key=60 sample=*silence
            <region> key=60 delay_cc132=1 sample=kick.wav trigger=release
            <region> key=61 sample=snare.wav
            <region> key=61 offset_cc132=100 sample=snare.wav trigger=release
        )");
        synth.hdNoteOn(0, 60, 1.0f);
        synth.hdNoteOff(1, 60, 0.0f);
        synth.hdNoteOn(2, 60, 1.0f);
        synth.hdNoteOff(3, 60, 0.5f);
        synth.hdNoteOn(4, 61, 1.0f);
        synth.hdNoteOff(5, 61, 0.0f);
        synth.hdNoteOn(6, 61, 1.0f);
        synth.hdNoteOff(7, 61, 0.5f);
        synth.dispatchMessage(client, 10, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 10, "/voice3/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 10, "/voice5/source_position", "", nullptr);
        synth.dispatchMessage(client, 10, "/voice7/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice1/remaining_delay,i : { 1 }", // 1 is the note off event delay
            "/voice3/remaining_delay,i : { 24003 }", // 3 is the note off event delay
            "/voice5/source_position,i : { 0 }",
            "/voice7/source_position,i : { 50 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC133 - Note number")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> delay_cc133=1 offset_cc133=100 sample=kick.wav
        )");
        synth.hdNoteOn(0, 0, 1.0f);
        synth.hdNoteOn(0, 127, 1.0f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 0 }",
            "/voice1/remaining_delay,i : { 48000 }",
            "/voice0/source_position,i : { 0 }",
            "/voice1/source_position,i : { 100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC134 - Note gate")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> delay_cc134=1 offset_cc134=100 sample=kick.wav
        )");
        synth.hdNoteOn(0, 60, 1.0f);
        synth.hdNoteOn(0, 127, 1.0f);
        synth.hdNoteOff(1, 60, 1.0f);
        synth.hdNoteOff(1, 127, 1.0f);
        synth.hdNoteOn(2, 60, 1.0f);
        synth.hdNoteOn(2, 127, 1.0f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice3/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice3/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 0 }",
            "/voice1/remaining_delay,i : { 48000 }",
            "/voice2/remaining_delay,i : { 2 }", // 2 is the event delay
            "/voice3/remaining_delay,i : { 48002 }", // 2 is the event delay
            "/voice0/source_position,i : { 0 }",
            "/voice1/source_position,i : { 100 }",
            "/voice2/source_position,i : { 0 }",
            "/voice3/source_position,i : { 100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC137 - Alternate")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> delay_cc137=1 offset_cc137=100 sample=kick.wav
        )");
        synth.hdNoteOn(0, 60, 1.0f);
        synth.hdNoteOn(0, 127, 1.0f);
        synth.hdNoteOn(0, 54, 1.0f);
        synth.hdNoteOn(0, 12, 1.0f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice3/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice3/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 0 }",
            "/voice1/remaining_delay,i : { 48000 }",
            "/voice2/remaining_delay,i : { 0 }",
            "/voice3/remaining_delay,i : { 48000 }",
            "/voice0/source_position,i : { 0 }",
            "/voice1/source_position,i : { 100 }",
            "/voice2/source_position,i : { 0 }",
            "/voice3/source_position,i : { 100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC140 - Keydelta")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> delay=2 offset=200 delay_cc140=1 offset_cc140=100 sample=kick.wav
        )");
        synth.hdNoteOn(0, 60, 1.0f);
        synth.hdNoteOn(0, 61, 1.0f);
        synth.hdNoteOn(0, 59, 1.0f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 96000 }",
            "/voice1/remaining_delay,i : { 144000 }",
            "/voice2/remaining_delay,i : { 0 }",
            "/voice0/source_position,i : { 200 }",
            "/voice1/source_position,i : { 300 }",
            "/voice2/source_position,i : { 0 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC141 - Absolute Keydelta")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/extended_ccs.sfz", R"(
            <region> delay=2 offset=200 delay_cc141=1 offset_cc141=100 sample=kick.wav
        )");
        synth.hdNoteOn(0, 60, 1.0f);
        synth.hdNoteOn(0, 61, 1.0f);
        synth.hdNoteOn(0, 59, 1.0f);
        synth.dispatchMessage(client, 0, "/voice0/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/remaining_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice0/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice1/source_position", "", nullptr);
        synth.dispatchMessage(client, 0, "/voice2/source_position", "", nullptr);
        std::vector<std::string> expected {
            "/voice0/remaining_delay,i : { 96000 }",
            "/voice1/remaining_delay,i : { 144000 }",
            "/voice2/remaining_delay,i : { 192000 }",
            "/voice0/source_position,i : { 200 }",
            "/voice1/source_position,i : { 300 }",
            "/voice2/source_position,i : { 400 }",
        };
        REQUIRE(messageList == expected);
    }
}
