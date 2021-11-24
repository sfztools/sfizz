// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Wavetables.h"
#include "sfizz/FileMetadata.h"
#include "sfizz/MathHelpers.h"
#include "catch2/catch.hpp"
#include <iostream>
#include <fstream>
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
    sfz::FileMetadataReader reader { "tests/TestFiles/wavetables/surge.wav" };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Wavetable sound files: Clm")
{
    sfz::FileMetadataReader reader { "tests/TestFiles/wavetables/clm.wav" };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Non-wavetable sound files")
{
    sfz::FileMetadataReader reader { "tests/TestFiles/snare.wav" };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(!reader.extractWavetableInfo(wt));
}

std::vector<char> readWholeFile(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
        buffer.clear();

    return buffer;

}

TEST_CASE("[Wavetables] Wavetable sound files: Surge, from memory")
{
    auto file = readWholeFile("tests/TestFiles/wavetables/surge.wav");
    sfz::MemoryMetadataReader reader { file.data(), file.size() };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Wavetable sound files: Clm, from memory")
{
    auto file = readWholeFile("tests/TestFiles/wavetables/clm.wav");
    sfz::MemoryMetadataReader reader { file.data(), file.size() };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(reader.extractWavetableInfo(wt));

    REQUIRE(wt.tableSize == 256);
}

TEST_CASE("[Wavetables] Non-wavetable sound files, from memory")
{
    auto file = readWholeFile("tests/TestFiles/snare.wav");
    sfz::MemoryMetadataReader reader { file.data(), file.size() };
    sfz::WavetableInfo wt;

    REQUIRE(reader.open());
    REQUIRE(!reader.extractWavetableInfo(wt));
}
