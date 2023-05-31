// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioSpan.h"
#include "absl/types/span.h"
#include "sfizz/Synth.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"
#include <algorithm>
#include <vector>
using namespace Catch::literals;

TEST_CASE("[AudioFiles] No leakage on right")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    std::vector<float> zeros(synth.getSamplesPerBlock(), 0);
    sfz::AudioSpan<float> span { buffer };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/wavpack.sfz", R"(
        <region> sample=kick.wav key=60 pan=-100
    )");
    synth.noteOn(0, 60, 127);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    REQUIRE( approxEqual(span.getConstSpan(1), absl::MakeConstSpan(zeros), 1e-2f) );
    while (numPlayingVoices(synth) > 0) {
        synth.renderBlock(buffer);
        REQUIRE( approxEqual(span.getConstSpan(1), absl::MakeConstSpan(zeros), 1e-2f) );
    }
}

TEST_CASE("[AudioFiles] WavPack file")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    sfz::AudioSpan<float> span { buffer };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/wavpack.sfz", R"(
        <region> sample=kick.wav key=60 pan=-100
        <region> sample=kick.wv key=60 pan=100
    )");
    synth.noteOn(0, 60, 127);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( approxEqual(span.getConstSpan(0), span.getConstSpan(1)) );
    while (numPlayingVoices(synth) > 0) {
        synth.renderBlock(buffer);
        REQUIRE( approxEqual(span.getConstSpan(0), span.getConstSpan(1)) );
    }
}

TEST_CASE("[AudioFiles] Flac file")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    sfz::AudioSpan<float> span { buffer };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/wavpack.sfz", R"(
        <region> sample=kick.wav key=60 pan=-100
        <region> sample=kick.flac key=60 pan=100
    )");
    synth.noteOn(0, 60, 127);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( approxEqual(span.getConstSpan(0), span.getConstSpan(1)) );
    while (numPlayingVoices(synth) > 0) {
        synth.renderBlock(buffer);
        REQUIRE( approxEqual(span.getConstSpan(0), span.getConstSpan(1)) );
    }
}
