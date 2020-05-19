// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Tuning.h"
#include "sfizz/MathHelpers.h"
#include "catch2/catch.hpp"

TEST_CASE("[Tuning] Default tuning")
{
    sfz::Tuning defaultTuning;

    for (int key = 0; key < 128; ++key)
        REQUIRE(defaultTuning.getFrequencyOfKey(key) == Approx(midiNoteFrequency(key)));
}
