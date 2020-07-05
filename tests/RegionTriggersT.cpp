// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("Basic triggers", "Region triggers")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };

    region.parseOpcode({ "sample", "*sine" });
    SECTION("key")
    {
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!region.registerCC(63, 64_norm));
    }
    SECTION("lokey and hikey")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "42" });
        REQUIRE(!region.registerNoteOn(39, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOn(42, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(43, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(42, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(42, 64_norm, 0.5f));
        REQUIRE(!region.registerCC(63, 64_norm));
    }
    SECTION("key and release trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(41, 64_norm, 0.5f));
        REQUIRE(!region.registerCC(63, 64_norm));
    }
    SECTION("key and release_key trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release_key" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOff(40, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(41, 64_norm, 0.5f));
        REQUIRE(!region.registerNoteOff(41, 64_norm, 0.5f));
        REQUIRE(!region.registerCC(63, 64_norm));
    }
    // TODO: first and legato triggers
    SECTION("lovel and hivel")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lovel", "60" });
        region.parseOpcode({ "hivel", "70" });
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.5f));
        REQUIRE(region.registerNoteOn(40, 60_norm, 0.5f));
        REQUIRE(region.registerNoteOn(40, 70_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(41, 71_norm, 0.5f));
        REQUIRE(!region.registerNoteOn(41, 59_norm, 0.5f));
    }

    SECTION("lorand and hirand")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.34f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.35f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.36f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.37f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.38f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.39f));
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.40f));
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.41f));
    }

    SECTION("lorand and hirand on 1.0f")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.34f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.35f));
        REQUIRE(region.registerNoteOn(40, 64_norm, 1.0f));
    }

    SECTION("Disable key trigger")
    {
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "hikey", "-1" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "hikey", "40" });
        REQUIRE(region.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "key", "-1" });
        REQUIRE(!region.registerNoteOn(40, 64_norm, 1.0f));
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(40, 64_norm, 1.0f));
    }

    SECTION("on_loccN, on_hiccN")
    {
        region.parseOpcode({ "on_locc47", "64" });
        region.parseOpcode({ "on_hicc47", "68" });
        REQUIRE(!region.registerCC(47, 63_norm));
        REQUIRE(region.registerCC(47, 64_norm));
        REQUIRE(region.registerCC(47, 65_norm));
        region.parseOpcode({ "hikey", "-1" });
        REQUIRE(region.registerCC(47, 64_norm));
        REQUIRE(region.registerCC(47, 65_norm));
        REQUIRE(region.registerCC(47, 66_norm));
        REQUIRE(region.registerCC(47, 67_norm));
        REQUIRE(region.registerCC(47, 68_norm));
        REQUIRE(!region.registerCC(47, 69_norm));
        REQUIRE(!region.registerCC(40, 64_norm));
    }
}

TEST_CASE("Legato triggers", "Region triggers")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    SECTION("First note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "first" });
        midiState.noteOnEvent(0, 40, 64_norm);
        REQUIRE(region.registerNoteOn(40, 64_norm, 0.5f));
        midiState.noteOnEvent(0, 41, 64_norm);
        REQUIRE(!region.registerNoteOn(41, 64_norm, 0.5f));
        midiState.noteOffEvent(0, 40, 0_norm);
        region.registerNoteOff(40, 0_norm, 0.5f);
        midiState.noteOffEvent(0, 41, 0_norm);
        region.registerNoteOff(41, 0_norm, 0.5f);
        midiState.noteOnEvent(0, 42, 64_norm);
        REQUIRE(region.registerNoteOn(42, 64_norm, 0.5f));
    }

    SECTION("Second note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "legato" });
        midiState.noteOnEvent(0, 40, 64_norm);
        REQUIRE(!region.registerNoteOn(40, 64_norm, 0.5f));
        midiState.noteOnEvent(0, 41, 64_norm);
        REQUIRE(region.registerNoteOn(41, 64_norm, 0.5f));
        midiState.noteOffEvent(0, 40, 64_norm);
        region.registerNoteOff(40, 0_norm, 0.5f);
        midiState.noteOffEvent(0, 41, 64_norm);
        region.registerNoteOff(41, 0_norm, 0.5f);
        midiState.noteOnEvent(0, 42, 64_norm);
        REQUIRE(!region.registerNoteOn(42, 64_norm, 0.5f));
    }
}
