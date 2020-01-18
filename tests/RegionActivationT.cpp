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

#include "sfizz/Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("Region activation", "Region tests")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };

    region.parseOpcode({ "sample", "*sine" });
    SECTION("Basic state")
    {
        region.registerCC(4, 0);
        REQUIRE(region.isSwitchedOn());
    }

    SECTION("Single CC range")
    {
        region.parseOpcode({ "locc4", "56" });
        region.parseOpcode({ "hicc4", "59" });
        region.registerCC(4, 0);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 57);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 56);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 59);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 43);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 65);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(6, 57);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Multiple CC ranges")
    {
        region.parseOpcode({ "locc4", "56" });
        region.parseOpcode({ "hicc4", "59" });
        region.parseOpcode({ "locc54", "18" });
        region.parseOpcode({ "hicc54", "27" });
        region.registerCC(4, 0);
        region.registerCC(54, 0);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(4, 57);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(54, 19);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 18);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 27);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 56);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 59);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(54, 2);
        REQUIRE(!region.isSwitchedOn());
        region.registerCC(54, 26);
        REQUIRE(region.isSwitchedOn());
        region.registerCC(4, 65);
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
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0, 0.5f);
    }

    SECTION("Keyswitches: sw_last with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_last", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(60, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(60, 0, 0.5f);
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0, 0.5f);
    }

    SECTION("Keyswitches: sw_down with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_down", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(60, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(60, 0, 0.5f);
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0, 0.5f);
    }

    SECTION("Keyswitches: sw_up with non-default keyswitch range")
    {
        region.parseOpcode({ "sw_lokey", "30" });
        region.parseOpcode({ "sw_hikey", "50" });
        region.parseOpcode({ "sw_up", "40" });
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        region.registerNoteOff(41, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }

    SECTION("Keyswitches: sw_previous")
    {
        region.parseOpcode({ "sw_previous", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        region.registerNoteOff(41, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(41, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(41, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
    }

    SECTION("Sequences: length 2, default position")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "1" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
    }
    SECTION("Sequences: length 2, position 2")
    {
        region.parseOpcode({ "seq_length", "2" });
        region.parseOpcode({ "seq_position", "2" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }
    SECTION("Sequences: length 3, position 2")
    {
        region.parseOpcode({ "seq_length", "3" });
        region.parseOpcode({ "seq_position", "2" });
        region.parseOpcode({ "key", "40" });
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(!region.isSwitchedOn());
        region.registerNoteOn(40, 64, 0.5f);
        REQUIRE(region.isSwitchedOn());
        region.registerNoteOff(40, 0, 0.5f);
        REQUIRE(region.isSwitchedOn());
    }
}
