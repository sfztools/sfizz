// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Wavetables.h"
#include "sfizz/FileMetadata.h"
#include "sfizz/MathHelpers.h"
#include "catch2/catch.hpp"
#include <algorithm>
#include <cmath>

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

TEST_CASE("[Wavetables] Octave number lookup")
{
    for (int note = 0; note < 128; ++note) {
        double f = midiNoteFrequency(note);

        float ref = std::log2(f * sfz::WavetableRange::frequencyScaleFactor);
        float oct = sfz::WavetableRange::getFractionalOctaveForFrequency(f);

        ref = clamp<float>(ref, 0, sfz::WavetableRange::countOctaves - 1);
        oct = clamp<float>(oct, 0, sfz::WavetableRange::countOctaves - 1);

        REQUIRE(oct == Approx(ref).margin(0.03f));
    }
}

TEST_CASE("[Wavetables] Wavetable sound files: Surge")
{
    sfz::FileMetadataReader reader;
    sfz::WavetableInfo wt;

    REQUIRE(reader.open("tests/TestFiles/wavetables/surge.wav"));
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Wavetable sound files: Clm")
{
    sfz::FileMetadataReader reader;
    sfz::WavetableInfo wt;

    REQUIRE(reader.open("tests/TestFiles/wavetables/clm.wav"));
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Non-wavetable sound files")
{
    sfz::FileMetadataReader reader;
    sfz::WavetableInfo wt;

    REQUIRE(reader.open("tests/TestFiles/snare.wav"));
    REQUIRE(!reader.extractWavetableInfo(wt));
}
