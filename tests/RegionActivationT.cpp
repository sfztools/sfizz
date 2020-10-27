// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "sfizz/Synth.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("Region activation", "Region tests")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };

    region.parseOpcode({ "sample", "*sine" });
    SECTION("Basic state")
    {
        region.registerCC(4, 0_norm);
        REQUIRE(region.isSwitchedOn());
    }

    SECTION("Single CC range")
    {
        region.parseOpcode({ "locc4", "56" });
        region.parseOpcode({ "hicc4", "59" });
        region.registerCC(4, 0_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 57_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 56_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 59_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 43_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 65_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(6, 57_norm);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Multiple CC ranges")
    {
        region.parseOpcode({ "locc4", "56" });
        region.parseOpcode({ "hicc4", "59" });
        region.parseOpcode({ "locc54", "18" });
        region.parseOpcode({ "hicc54", "27" });
        region.registerCC(4, 0_norm);
        region.registerCC(54, 0_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 57_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(54, 19_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 18_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 27_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 56_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 59_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 2_norm);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(54, 26_norm);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 65_norm);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Bend ranges")
    {
        region.parseOpcode({ "lobend", "56" });
        region.parseOpcode({ "hibend", "243" });
        region.registerPitchWheel(0);
        REQUIRE(!region.isSwitchedOn());
        region.registerPitchWheel(sfz::normalizeBend(56));
        REQUIRE(region.isSwitchedOn());
        region.registerPitchWheel(sfz::normalizeBend(243));
        REQUIRE(region.isSwitchedOn());
        region.registerPitchWheel(sfz::normalizeBend(245));
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Aftertouch ranges")
    {
        region.parseOpcode({ "lochanaft", "56" });
        region.parseOpcode({ "hichanaft", "68" });
        region.registerAftertouch(0);
        REQUIRE(!region.isSwitchedOn());
        region.registerAftertouch(56);
        REQUIRE(region.isSwitchedOn());
        region.registerAftertouch(68);
        REQUIRE(region.isSwitchedOn());
        region.registerAftertouch(98);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("BPM ranges")
    {
        region.parseOpcode({ "lobpm", "56" });
        region.parseOpcode({ "hibpm", "68" });
        region.registerTempo(2.0f);
        REQUIRE(!region.isSwitchedOn());
        region.registerTempo(0.90f);
        REQUIRE(region.isSwitchedOn());
        region.registerTempo(1.01f);
        REQUIRE(region.isSwitchedOn());
        region.registerTempo(1.1f);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Sequences: length 2, default position")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "1" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }
    SECTION("Sequences: length 2, position 2")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "2" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
    }
    SECTION("Sequences: length 3, position 2")
    {
        region.parseOpcode({ "seq_length", "3" });
        region.parseOpcode({ "seq_position", "2" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
    }
}

TEST_CASE("[Keyswitches] Normal lastKeyswitch range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=40 sw_hikey=42 sw_default=40
        <region> sw_last=40 key=60 sample=*sine
        <region> sw_last=41 key=62 sample=*saw
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 41, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Keyswitches] No lastKeyswitch range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <region> sw_last=40 key=60 sample=*sine
        <region> sw_last=41 key=62 sample=*saw
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 41, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Keyswitches] Out of lastKeyswitch range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=40 sw_hikey=42 sw_default=40
        <region> sw_last=40 key=60 sample=*sine
        <region> sw_last=43 key=62 sample=*saw
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 43, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Keyswitches] Overlapping key and lastKeyswitch range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=1 sw_hikey=127 sw_default=40
        <region> sw_last=40 key=60 sample=*sine
        <region> sw_last=41 key=62 sample=*saw
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 41, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
    synth.noteOn(0, 43, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
    synth.noteOn(0, 62, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 3);
}

TEST_CASE("[Keyswitches] sw_down, in range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=1 sw_hikey=127 sw_default=40
        <region> sw_down=40 key=60 sample=*sine
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOff(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
}

TEST_CASE("[Keyswitches] sw_down, out of range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=1 sw_hikey=10 sw_default=40
        <region> sw_down=40 key=60 sample=*sine
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOff(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
}

TEST_CASE("[Keyswitches] sw_up, in range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=1 sw_hikey=127 sw_default=40
        <region> sw_up=40 key=60 sample=*sine
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOff(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Keyswitches] sw_up, out of range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/keyswitches.sfz", R"(
        <global> sw_lokey=1 sw_hikey=127 sw_default=40
        <region> sw_up=40 key=60 sample=*sine
    )");
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOn(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    synth.noteOff(0, 40, 64);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Keyswitches] sw_default")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_default.sfz", R"(
        <global> sw_lokey=30 sw_hikey=50 sw_default=40
        <region> sw_last=41 key=51 sample=*sine
        <region> sw_last=40 key=52 sample=*sine
        <region> sw_last=41 key=53 sample=*sine
        <region> sw_last=40 key=54 sample=*sine
    )");
    REQUIRE( synth.getNumRegions() == 4 );
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
}

TEST_CASE("[Keyswitches] sw_default and playing with switches")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_default.sfz", R"(
        <global> sw_lokey=30 sw_hikey=50 sw_default=40
        <region> sw_last=41 key=51 sample=*sine
        <region> sw_last=40 key=52 sample=*sine
        <region> sw_last=41 key=53 sample=*sine
        <region> sw_last=40 key=54 sample=*sine
    )");
    REQUIRE( synth.getNumRegions() == 4 );
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
    synth.noteOn(0, 41, 64);
    synth.noteOff(0, 41, 0);
    REQUIRE( synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(3)->isSwitchedOn() );
    synth.noteOn(0, 40, 64);
    synth.noteOff(0, 40, 64);
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
}

TEST_CASE("[Keyswitches] sw_previous in range")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_previous.sfz", R"(
        <region> sample=*saw sw_previous=60 lokey=50 hikey=70
    )");
    // Note: sforzando seems to activate by default if sw_previous is indeed 60,
    // but not any other value. As it does not seem really useful at this point
    // the test assumes that sw_previous regions are disabled by default
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 51, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 51, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
}

TEST_CASE("[Keyswitches] sw_previous out of range")
{
    // The behavior is the same in this case, regardless of the keyrange
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_previous.sfz", R"(
        <region> sample=*saw sw_previous=60 lokey=50 hikey=55
    )");
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 51, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 51, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 60, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 1);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    synth.noteOn(0, 61, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
}

TEST_CASE("[Keyswitches] sw_lolast and sw_hilast")
{
    // The behavior is the same in this case, regardless of the keyrange
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_previous.sfz", R"(
        <region> sw_lolast=57 sw_hilast=59 key=70 sample=*saw
        <region> sw_lolast=60 sw_hilast=62 key=72 sample=*sine
    )");
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 51, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 57, 64);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 60, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 58, 64);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 61, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 59, 64);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 62, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(synth.getRegionView(1)->isSwitchedOn());
}

TEST_CASE("[Keyswitches] sw_lolast and sw_hilast with sw_last")
{
    // The behavior is the same in this case, regardless of the keyrange
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sw_previous.sfz", R"(
        <region> sw_last=40 sw_lolast=57 sw_hilast=59 key=70 sample=*saw
        <region> sw_lolast=60 sw_hilast=62 sw_last=41 key=72 sample=*sine
    )");
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 40, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 41, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 57, 64);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 41, 64);
    REQUIRE(synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(!synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 60, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(synth.getRegionView(1)->isSwitchedOn());
    synth.noteOn(0, 40, 64);
    REQUIRE(!synth.getRegionView(0)->isSwitchedOn());
    REQUIRE(synth.getRegionView(1)->isSwitchedOn());
}
