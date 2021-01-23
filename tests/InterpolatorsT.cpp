// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Interpolators.h"
#include "catch2/catch.hpp"
#include <array>
#include <numeric>
using namespace Catch::literals;

TEST_CASE("[Interpolators] Sample at points")
{
    std::array<float, 32> values;
    std::iota(values.begin(), values.end(), 0.0f);
    for (unsigned i = 2; i < values.size() - 2; ++i) {
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 0.0f, 1.0f)
            == Approx(values[i]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 0.0f, 1.0f)
            == Approx(values[i]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 0.0f, 1.0f)
            == Approx(values[i]).margin(1e-2));
    }
}

TEST_CASE("[Interpolators] Sample next")
{
    std::array<float, 32> values;
    std::iota(values.begin(), values.end(), 0.0f);
    for (unsigned i = 2; i < values.size() - 2; ++i) {
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 1.0f, 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 1.0f, 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 1.0f, 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
    }
}

TEST_CASE("[Interpolators] Straight line")
{
    std::array<float, 32> values;
    std::iota(values.begin(), values.end(), 0.0f);
    for (unsigned i = 2; i < values.size() - 2; ++i) {
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 0.5f, 1.0f)
            == Approx(values[i] + 0.5f).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 0.5f, 1.0f)
            == Approx(values[i] + 0.5f).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 0.5f, 1.0f)
            == Approx(values[i] + 0.5f).margin(1e-2));
    }
}

TEST_CASE("[Interpolators] Squares")
{
    std::array<float, 32> x;
    std::array<float, 32> y;
    for (unsigned i = 0; i < x.size(); ++i) {
        x[i] = static_cast<float>(i) / static_cast<float>(x.size());
        y[i] = x[i] * x[i];
    }

    for (unsigned i = 2; i < x.size() - 2; ++i) {
        const auto half_x = x[i] + 0.5f / static_cast<float>(x.size());
        const auto expected = (half_x) * (half_x);
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&y[i], 0.5f, 1.0f)
            == Approx(expected).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&y[i], 0.5f, 1.0f)
            == Approx(expected).margin(1e-2));
    }
}
