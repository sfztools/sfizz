// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("Region activation", "Region tests")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };

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
        region.registerPitchWheel(56);
        REQUIRE(region.isSwitchedOn());
        region.registerPitchWheel(243);
        REQUIRE(region.isSwitchedOn());
        region.registerPitchWheel(245);
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

    // TODO: add keyswitches
    SECTION("Keyswitches: sw_last")
    {
        region.parseOpcode({ "sw_last", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0_norm, 0.5f);
    }

    SECTION("Keyswitches: sw_last with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_last", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(60, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(60, 0_norm, 0.5f);
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0_norm, 0.5f);
    }

    SECTION("Keyswitches: sw_down with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_down", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0_norm, 0.5f);
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0_norm, 0.5f);
    }

    SECTION("Keyswitches: sw_up with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_up", "40" });
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        region.registerNoteOff(41, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }

    SECTION("Keyswitches: sw_previous")
    {
        region.parseOpcode({ "sw_previous", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        region.registerNoteOff(41, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Sequences: length 2, default position")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "1" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.isSwitchedOn());
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
    SECTION("Sequences: length 2, position 2")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "2" });
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
    SECTION("Sequences: length 3, position 2")
    {
        region.parseOpcode({ "seq_length", "3" });
        region.parseOpcode({ "seq_position", "2" });
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
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0_norm, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }
}
