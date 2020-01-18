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
