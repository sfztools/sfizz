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
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
#include "absl/strings/string_view.h"
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

TEST_CASE("[MidiState] Reset")
{
    sfz::MidiState state;
    state.pitchBendEvent(0, 0.7f);
    state.noteOnEvent(0, 64, 24_norm);
    state.ccEvent(0, 123, 124_norm);
    state.reset();
    REQUIRE(state.getPitchBend() == 0.0f);
    REQUIRE(state.getNoteVelocity(64) == 0_norm);
    REQUIRE(state.getCCValue(123) == 0_norm);
}

TEST_CASE("[MidiState] Reset all controllers")
{
    sfz::MidiState state;
    state.pitchBendEvent(20, 0.7f);
    state.ccEvent(10, 122, 124_norm);
    REQUIRE(state.getPitchBend() == 0.7f);
    REQUIRE(state.getCCValue(122) == 124_norm);
    state.resetAllControllers(30);
    REQUIRE(state.getPitchBend() == 0.0f);
    REQUIRE(state.getCCValue(122) == 0_norm);
    REQUIRE(state.getCCValue(4) == 0_norm);
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
    REQUIRE(state.getLastVelocity() == 10_norm);
}
