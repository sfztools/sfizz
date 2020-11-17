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

TEST_CASE("[Tuning] Railsback disabled")
{
    auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.0f);
    for (int key = 0; key < 128; ++key)
        REQUIRE(tuning.getRatioForIntegralKey(key) == 1.0f);
}

TEST_CASE("[Tuning] Stretch integral == float")
{
    auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.25f);
    for (int key = 0; key < 128; ++key)
        REQUIRE(tuning.getRatioForIntegralKey(key) == tuning.getRatioForFractionalKey(static_cast<float>(key)));
}

TEST_CASE("[Tuning] Stretch definition points")
{
    SECTION("For 0.5f")
    {
        static constexpr float railsback[128] = {
            #include "sfizz/railsback/4-1.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.5f);
        for (int key = 0; key < 128; ++key)
            REQUIRE(tuning.getRatioForIntegralKey(key) == railsback[key]);
    }
    SECTION("For 1.0f")
    {
        static constexpr float railsback[128] = {
            #include "sfizz/railsback/4-2.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(1.0f);
        for (int key = 0; key < 128; ++key)
            REQUIRE(tuning.getRatioForIntegralKey(key) == railsback[key]);
    }

    SECTION("For 0.25f")
    {
        static constexpr float railsback[128] = {
            #include "sfizz/railsback/2-1.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.25f);
        for (int key = 0; key < 128; ++key)
            REQUIRE(tuning.getRatioForIntegralKey(key) == railsback[key]);
    }
}

TEST_CASE("[Tuning] Stretch interpolation bounds")
{
    SECTION("Between 0 and 0.25f")
    {
        static constexpr float bound1[128] = {
            #include "sfizz/railsback/2-1.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.1f);
        for (int key = 0; key < 128; ++key) {
            if (bound1[key] < 1.0f) {
                REQUIRE(tuning.getRatioForIntegralKey(key) >= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) <= 1.0f);
            } else {
                REQUIRE(tuning.getRatioForIntegralKey(key) <= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) >= 1.0f);
            }
        }
    }
    SECTION("Between 0.25f and 0.5f")
    {
        static constexpr float bound1[128] = {
            #include "sfizz/railsback/2-1.h"
        };
        static constexpr float bound2[128] = {
            #include "sfizz/railsback/4-1.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.3f);
        for (int key = 0; key < 128; ++key) {
            if (bound1[key] < bound2[key]) {
                REQUIRE(tuning.getRatioForIntegralKey(key) >= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) <= bound2[key]);
            } else {
                REQUIRE(tuning.getRatioForIntegralKey(key) <= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) >= bound2[key]);
            }
        }
    }
    SECTION("Between 0.5f and 1.0f")
    {
        static constexpr float bound1[128] = {
            #include "sfizz/railsback/4-1.h"
        };
        static constexpr float bound2[128] = {
            #include "sfizz/railsback/4-2.h"
        };
        auto tuning = sfz::StretchTuning::createRailsbackFromRatio(0.7f);
        for (int key = 0; key < 128; ++key) {
            if (bound1[key] < bound2[key]) {
                REQUIRE(tuning.getRatioForIntegralKey(key) >= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) <= bound2[key]);
            } else {
                REQUIRE(tuning.getRatioForIntegralKey(key) <= bound1[key]);
                REQUIRE(tuning.getRatioForIntegralKey(key) >= bound2[key]);
            }
        }
    }
}


