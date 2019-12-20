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

#include "Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("Basic triggers", "Region triggers")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };

    region.parseOpcode({ "sample", "*sine" });
    SECTION("key")
    {
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(0, 63, 64));
    }
    SECTION("lokey and hikey")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "42" });
        REQUIRE(!region.registerNoteOn(0, 39, 64, 0.5f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(0, 41, 64, 0.5f));
        REQUIRE(region.registerNoteOn(0, 42, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 43, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 42, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 42, 64, 0.5f));
        REQUIRE(!region.registerCC(0, 63, 64));
    }
    SECTION("key and release trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOff(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(0, 63, 64));
    }
    SECTION("key and release_key trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release_key" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOff(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(0, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(0, 63, 64));
    }
    // TODO: first and legato triggers
    SECTION("lovel and hivel")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lovel", "60" });
        region.parseOpcode({ "hivel", "70" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(0, 40, 60, 0.5f));
        REQUIRE(region.registerNoteOn(0, 40, 70, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 71, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 59, 0.5f));
    }
    SECTION("lochan and hichan")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lochan", "2" });
        region.parseOpcode({ "hichan", "4" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(2, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(3, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(4, 40, 64, 0.5f));
    }

    SECTION("lorand and hirand")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.34f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.35f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.36f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.37f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.38f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.39f));
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.40f));
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.41f));
    }

    SECTION("lorand and hirand on 1.0f")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.34f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.35f));
        REQUIRE(region.registerNoteOn(0, 40, 64, 1.0f));
    }

    SECTION("Disable key trigger")
    {
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 1.0f));
        region.parseOpcode({ "hikey", "-1" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 1.0f));
        region.parseOpcode({ "hikey", "40" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 1.0f));
        region.parseOpcode({ "key", "-1" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 1.0f));
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 1.0f));
    }

    SECTION("on_loccN, on_hiccN")
    {
        region.parseOpcode({ "on_locc47", "64" });
        region.parseOpcode({ "on_hicc47", "68" });
        REQUIRE(!region.registerCC(0, 47, 63));
        REQUIRE(!region.registerCC(0, 47, 64));
        REQUIRE(!region.registerCC(0, 47, 65));
        region.parseOpcode({ "hikey", "-1" });
        REQUIRE(region.registerCC(0, 47, 64));
        REQUIRE(region.registerCC(0, 47, 65));
        REQUIRE(region.registerCC(0, 47, 66));
        REQUIRE(region.registerCC(0, 47, 67));
        REQUIRE(region.registerCC(0, 47, 68));
        REQUIRE(!region.registerCC(0, 47, 69));
        REQUIRE(!region.registerCC(0, 40, 64));
    }
}

TEST_CASE("Legato triggers", "Region triggers")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    SECTION("First note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "first" });
        REQUIRE(region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(0, 41, 64, 0.5f));
        region.registerNoteOff(0, 40, 0, 0.5f);
        region.registerNoteOff(0, 41, 0, 0.5f);
        REQUIRE(region.registerNoteOn(0, 42, 64, 0.5f));
    }

    SECTION("Second note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "legato" });
        REQUIRE(!region.registerNoteOn(0, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(0, 41, 64, 0.5f));
        region.registerNoteOff(0, 40, 0, 0.5f);
        region.registerNoteOff(0, 41, 0, 0.5f);
        REQUIRE(!region.registerNoteOn(0, 42, 64, 0.5f));
    }
}
