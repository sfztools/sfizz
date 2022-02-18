// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/OversamplerHelpers.h"
#include "catch2/catch.hpp"

TEST_CASE("[Oversampler] Conversion factor")
{
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 44100.0) == 1);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 44101.0) == 2);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 88200.0) == 2);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 88201.0) == 4);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 176400.0) == 4);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 176401.0) == 8);
    // low and high limits
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 1.0) == 1);
    REQUIRE(sfz::Upsampler::conversionFactor(44100.0, 1e10) == 128);
}
