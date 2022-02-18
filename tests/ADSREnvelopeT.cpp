// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/ADSREnvelope.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <algorithm>
#include <array>
#include <iostream>
using namespace Catch::literals;

TEST_CASE("[ADSREnvelope] Basic state")
{
    sfz::ADSREnvelope envelope;
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.0, 0.0, 0.0, 0.0, 0.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Attack")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.5f, 1.0f, 1.0f, 1.0f, 1.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Attack again")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.03f;

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.33333f, 0.66667f, 1.0f, 1.0f, 1.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Release")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(2);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5f, 1.0f, 0.13534f, 0.018f, 0.0024f, 0.0f, 0.0f, 0.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(2);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Delay")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    std::array<float, 10> output;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(4);
    std::array<float, 10> expected { 0.0f, 0.0f, 0.5f, 1.0f, 0.13534f, 0.018f, 0.0024f, 0.0f, 0.0f, 0.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(4);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Lower sustain")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    region.amplitudeEG.sustain = 50.0f;
    std::array<float, 10> output;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    std::array<float, 10> expected { 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Decay")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    region.amplitudeEG.sustain = 50.0f;
    region.amplitudeEG.decay = 0.02f;
    std::array<float, 10> output;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    std::array<float, 10> expected { 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Hold")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    region.amplitudeEG.sustain = 50.0f;
    region.amplitudeEG.decay = 0.02f;
    region.amplitudeEG.hold = 0.02f;
    std::array<float, 12> output;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    std::array<float, 12> expected { 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.707107f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));

    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Hold with release")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    region.amplitudeEG.sustain = 50.0f;
    region.amplitudeEG.decay = 0.02f;
    region.amplitudeEG.hold = 0.02f;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(8);
    std::array<float, 15> output;
    std::array<float, 15> expected { 0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.707107f, 0.5f, 0.05f, 0.005f, 0.0005f, 0.00005f, 0.0f, 0.0f };
    envelope.getBlock(absl::MakeSpan(output));

    REQUIRE(approxEqual<float>(output, expected));
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(8);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[ADSREnvelope] Hold with release 2")
{
    sfz::ADSREnvelope envelope;
    sfz::MidiState state;
    sfz::Region region { state };
    region.amplitudeEG.attack = 0.02f;
    region.amplitudeEG.release = 0.04f;
    region.amplitudeEG.delay = 0.02f;
    region.amplitudeEG.sustain = 50.0f;
    region.amplitudeEG.decay = 0.02f;
    region.amplitudeEG.hold = 0.02f;
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(4);
    std::array<float, 15> output;
    std::array<float, 15> expected { 0.0f, 0.0f, 0.5f, 1.0f, 0.08409f, 0.00707f, 0.000594604f, 0.00005f, 0.0f, 0.0f, 0.0f, 0.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
    envelope.reset(region.amplitudeEG, region, state, 0, 0.0f, 100.0f);
    envelope.startRelease(4);
    absl::c_fill(output, -1.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}
