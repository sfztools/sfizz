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
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 0.0f)
            == Approx(values[i]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 0.0f)
            == Approx(values[i]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 0.0f)
            == Approx(values[i]).margin(1e-2));
    }
}

TEST_CASE("[Interpolators] Sample next")
{
    std::array<float, 32> values;
    std::iota(values.begin(), values.end(), 0.0f);
    for (unsigned i = 2; i < values.size() - 2; ++i) {
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 1.0f)
            == Approx(values[i + 1]).margin(1e-2));
    }
}

TEST_CASE("[Interpolators] Straight line")
{
    std::array<float, 32> values;
    std::iota(values.begin(), values.end(), 0.0f);
    for (unsigned i = 2; i < values.size() - 2; ++i) {
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorLinear>(&values[i], 0.5f)
            == Approx(values[i] + 0.5f).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&values[i], 0.5f)
            == Approx(values[i] + 0.5f).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&values[i], 0.5f)
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
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorHermite3>(&y[i], 0.5f)
            == Approx(expected).margin(1e-2));
        REQUIRE(sfz::interpolate<sfz::InterpolatorModel::kInterpolatorBspline3>(&y[i], 0.5f)
            == Approx(expected).margin(1e-2));
    }
}

template <class WS>
static std::pair<double, double> windowedSincError(WS& ws, double step = 0.1, bool verbose = false)
{
    size_t points = ws.getNumPoints();
    double x1 = points / -2.0;
    double x2 = points / +2.0;
    double maxAbsErr = 0.0;
    double meanAbsErr = 0.0;
    //double meanSquareErr = 0.0;

    double x;
    size_t n;
    for (n = 0; (x = x1 + n * step) < x2; ++n) {
        double val = ws.getUnchecked(x);
        double ref = ws.getExact(x);
        double absErr = std::fabs(val - ref);
        maxAbsErr = std::max(maxAbsErr, absErr);
        meanAbsErr += absErr;
        //meanSquareErr += absErr * absErr;
    }
    meanAbsErr /= n;
    //meanSquareErr /= n;

    if (verbose) {
        std::cerr << "MaxAbsErr=" << maxAbsErr
                  << " MeanAbsErr=" << meanAbsErr
                  //<< " MeanSquareErr=" << meanSquareErr
                  << " with Points=" << points
                  << " TableSize=" << ws.getTableSize()
                  << "\n";
    }

    return { maxAbsErr, meanAbsErr };
}

TEST_CASE("[Interpolators] Windowed sinc precision")
{
    sfz::initializeInterpolators();

    double maxAbsTolerance = 5e-2;
    double meanAbsTolerance = 1e-3;

    auto Check = [=](std::pair<double, double> maxAndMeanErr) {
        REQUIRE(maxAndMeanErr.first < maxAbsTolerance);
        REQUIRE(maxAndMeanErr.second < meanAbsTolerance);
    };

    Check(windowedSincError(*sfz::SincInterpolatorTraits<8>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<12>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<16>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<24>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<36>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<48>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<60>::windowedSinc));
    Check(windowedSincError(*sfz::SincInterpolatorTraits<72>::windowedSinc));
}
