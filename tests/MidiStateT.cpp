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
#include "catch2/catch.hpp"
#include "absl/strings/string_view.h"
using namespace Catch::literals;

TEST_CASE("[MidiState] Initial values")
{
    sfz::MidiState state;
    for (auto& cc: state.getCCArray())
        REQUIRE( cc == 0 );
    REQUIRE( state.getPitchBend() == 0 );
}

TEST_CASE("[MidiState] Set and get CCs")
{
    sfz::MidiState state;
    const auto& cc = state.getCCArray();
    state.ccEvent(24, 23);
    state.ccEvent(123, 124);
    REQUIRE(state.getCCValue(24) == 23);
    REQUIRE(cc[24] == 23);
    REQUIRE(state.getCCValue(123) == 124);
    REQUIRE(cc[123] == 124);
}

TEST_CASE("[MidiState] Set and get pitch bends")
{
    sfz::MidiState state;
    state.pitchBendEvent(894);
    REQUIRE(state.getPitchBend() == 894);
    state.pitchBendEvent(0);
    REQUIRE(state.getPitchBend() == 0);
}

TEST_CASE("[MidiState] Reset")
{
    sfz::MidiState state;
    state.pitchBendEvent(894);
    state.noteOnEvent(64, 24);
    state.ccEvent(123, 124);
    state.reset();
    REQUIRE(state.getPitchBend() == 0);
    REQUIRE(state.getNoteVelocity(64) == 0);
    REQUIRE(state.getCCValue(123) == 0);
}

TEST_CASE("[MidiState] Set and get note velocities")
{
    sfz::MidiState state;
    state.noteOnEvent(64, 24);
    REQUIRE(+state.getNoteVelocity(64) == 24);
    state.noteOnEvent(64, 123);
    REQUIRE(+state.getNoteVelocity(64) == 123);
}

TEST_CASE("[MidiState] Extended CCs")
{
    sfz::MidiState state;
    REQUIRE(state.getCCArray().size() >= 142);
    state.ccEvent(142, 64); // should not trap
}
