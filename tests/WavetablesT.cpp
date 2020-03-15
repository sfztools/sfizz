// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Wavetables.h"
#include "sfizz/MathHelpers.h"
#include "catch2/catch.hpp"
#include <algorithm>

TEST_CASE("[Wavetables] Frequency ranges")
{
    int cur_oct = std::numeric_limits<int>::min();
    int min_oct = std::numeric_limits<int>::max();
    int max_oct = std::numeric_limits<int>::min();

    for (int note = 0; note < 128; ++note) {
        double f = midiNoteFrequency(note);

        int oct = sfz::WavetableRange::getOctaveForFrequency(f);

        REQUIRE(oct >= 0);
        REQUIRE(oct < sfz::WavetableRange::countOctaves);

        REQUIRE(oct >= cur_oct);
        cur_oct = oct;

        min_oct = std::min(min_oct, oct);
        max_oct = std::max(max_oct, oct);

        sfz::WavetableRange range = sfz::WavetableRange::getRangeForOctave(oct);
        REQUIRE((f >= range.minFrequency || oct == 0));
        REQUIRE((f <= range.maxFrequency || oct == sfz::WavetableRange::countOctaves - 1));
    }

    // check ranges to be decently adjusted to the MIDI frequency range
    REQUIRE(min_oct == 0);
    REQUIRE(max_oct == sfz::WavetableRange::countOctaves - 1);
}
