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
    int cur_index = std::numeric_limits<int>::min();
    int min_index = std::numeric_limits<int>::max();
    int max_index = std::numeric_limits<int>::min();

    for (int note = 0; note < 128; ++note) {
        double f = midiNoteFrequency(note);

        float fractionalIndex = sfz::MipmapRange::getExactIndexForFrequency(f);
        int index = static_cast<int>(fractionalIndex);

        REQUIRE(index >= 0);
        REQUIRE(static_cast<unsigned>(index) < sfz::MipmapRange::N);

        float lerpFractionalIndex = sfz::MipmapRange::getIndexForFrequency(f);
        int lerpIndex = static_cast<int>(lerpFractionalIndex);

        // approximation should be equal or off by 1 table in worst cases
        bool lerpIndexValid = (lerpIndex - index) == 0 || (lerpIndex - index) == -1;
        REQUIRE(lerpIndexValid);

        REQUIRE(index >= cur_index);
        cur_index = index;

        min_index = std::min(min_index, index);
        max_index = std::max(max_index, index);

        sfz::MipmapRange range = sfz::MipmapRange::getRangeForIndex(index);
        REQUIRE((f >= range.minFrequency || index == 0));
        REQUIRE((f <= range.maxFrequency || index == sfz::MipmapRange::N - 1));
    }

    // check ranges to be decently adjusted to the MIDI frequency range
    REQUIRE(min_index == 0);
    REQUIRE(max_index == sfz::MipmapRange::N - 1);
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
