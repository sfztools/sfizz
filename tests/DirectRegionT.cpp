// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "TestHelpers.h"
#include "sfizz/MidiState.h"
#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "catch2/catch.hpp"
#include <stdexcept>
using namespace Catch::literals;
using namespace sfz::literals;
using namespace sfz;

TEST_CASE("[Direct Region Tests] amp_velcurve")
{
    MidiState midiState;
    Region region { 0, midiState };
    region.parseOpcode({ "amp_velcurve_6", "0.4" });
    REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(6, 0.4f));
    region.parseOpcode({ "amp_velcurve_127", "-1.0" });
    REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(127, -1.0f));
    region.parseOpcode({ "amp_velcurve_008", "0.3" });
    REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(8, 0.3f));
    region.parseOpcode({ "amp_velcurve_064", "0.9" });
    REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(64, 0.9f));
}

TEST_CASE("[Direct Region Tests] Release and release key")
{
    MidiState midiState;
    Region region { 0, midiState };
    region.parseOpcode({ "lokey", "63" });
    region.parseOpcode({ "hikey", "65" });
    region.parseOpcode({ "sample", "*sine" });
    SECTION("Release key without sustain")
    {
        region.parseOpcode({ "trigger", "release_key" });
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
    }
    SECTION("Release key with sustain")
    {
        region.parseOpcode({ "trigger", "release_key" });
        midiState.ccEvent(0, 64, 1.0f);
        REQUIRE( !region.registerCC(64, 1.0f) );
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
    }

    SECTION("Release without sustain")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
    }

    SECTION("Release with sustain")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 1.0f);
        midiState.noteOnEvent(0, 63, 0.5f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( !region.registerNoteOff(63, 0.5f, 0.0f) );
        REQUIRE( region.delayedReleases.size() == 1 );
        std::vector<std::pair<int, float>> expected = {
            { 63, 0.5f }
        };
        REQUIRE( region.delayedReleases == expected );
    }

    SECTION("Release with sustain and 2 notes")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 1.0f);
        midiState.noteOnEvent(0, 63, 0.5f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        midiState.noteOnEvent(0, 64, 0.6f);
        REQUIRE( !region.registerNoteOn(64, 0.6f, 0.0f) );
        REQUIRE( !region.registerNoteOff(63, 0.0f, 0.0f) );
        REQUIRE( !region.registerNoteOff(64, 0.2f, 0.0f) );
        REQUIRE( region.delayedReleases.size() == 2 );
        std::vector<std::pair<int, float>> expected = {
            { 63, 0.5f },
            { 64, 0.6f }
        };
        REQUIRE( region.delayedReleases == expected );
    }

    SECTION("Release with sustain and 2 notes but 1 outside")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 1.0f);
        midiState.noteOnEvent(0, 63, 0.5f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        midiState.noteOnEvent(0, 66, 0.6f);
        REQUIRE( !region.registerNoteOn(66, 0.6f, 0.0f) );
        REQUIRE( !region.registerNoteOff(63, 0.0f, 0.0f) );
        REQUIRE( !region.registerNoteOff(66, 0.2f, 0.0f) );
        REQUIRE( region.delayedReleases.size() == 1 );
        std::vector<std::pair<int, float>> expected = {
            { 63, 0.5f }
        };
        REQUIRE( region.delayedReleases == expected );
    }
}
