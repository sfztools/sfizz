// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include "sfizz/MathHelpers.h"
#include <vector>

template <class T>
static inline T squared(T x)
{
    return x * x;
}

/**
 * @brief Check the behavior of a uniform real random generator
 *
 * - ensure all results to be in range [min;max]
 * - ensure to have at least one element in every division of the result range
 */
template <class Rand, class Dist>
static bool uniformRandomTest(float min, float max, unsigned gen, unsigned div)
{
    fast_rand prng;
    fast_real_distribution<float> dist(min, max);
    std::vector<unsigned> counts(div);

    for (unsigned i = 0; i < gen; ++i) {
        float r = dist(prng);
        if (r < min || r > max)
            return false;
        unsigned d = clamp<int>(div * (r - min) / (max - min), 0, div - 1);
        ++counts[d];
    }

    for (unsigned count : counts) {
        if (count == 0)
            return false;
    }
    return true;
}

TEST_CASE("[Random] Fast random generation")
{
    unsigned numGenerations = 1000;
    unsigned numDivisions = 128;

    auto& test = uniformRandomTest<fast_rand, fast_real_distribution<float>>;

    REQUIRE(test(0.0f, 1.0f, numGenerations, numDivisions));
    REQUIRE(test(-1.0f, 1.0f, numGenerations, numDivisions));
    REQUIRE(test(0.0f, 123.0f, numGenerations, numDivisions));
    REQUIRE(test(-123.0f, 0.0f, numGenerations, numDivisions));
}

/**
 * @brief Check the behavior of a gaussian real random generator
 *
 * - compute a histogram of random generations
 * - compare it against the standard library normal distribution
 */
template <unsigned Quality>
static bool gaussianRandomTest(double mean, float variance, size_t numGen, size_t histSize, double maxAbsErr)
{
    fast_gaussian_generator<float, Quality> gen(mean, variance);

    std::mt19937_64 prng;
    std::normal_distribution<float> dist(mean, variance);

    // the tested bounds
    const double min = -1.0 + mean;
    const double max = +1.0 + mean;

    // generate, quantify, count occurrences
    std::vector<size_t> counts(histSize);
    for (size_t i = 0; i < numGen; ++i) {
        double value = gen();
        double normalized = (value - min) / (max - min);
        long bin = std::lround(histSize * normalized);
        if (bin >= 0 && static_cast<size_t>(bin) < histSize)
            counts[bin] += 1;
    }
    std::vector<size_t> referenceCounts(histSize);
    for (size_t i = 0; i < numGen; ++i) {
        double value = dist(prng);
        double normalized = (value - min) / (max - min);
        long bin = std::lround(histSize * normalized);
        if (bin >= 0 && static_cast<size_t>(bin) < histSize)
            referenceCounts[bin] += 1;
    }

    // bin probabilities
    std::vector<double> proba(histSize);
    std::vector<double> referenceProba(histSize);
    for (size_t i = 0; i < histSize; ++i) {
        proba[i] = static_cast<double>(counts[i]) / numGen;
        referenceProba[i] = static_cast<double>(referenceCounts[i]) / numGen;
    }

    // normalize to unity, for the sake of comparison
    {
        double sum = 0.0;
        for (double x : referenceProba)
            sum += x;
        double scaleFactor = 1.0 / sum;
        for (size_t i = 0; i < histSize; ++i) {
            proba[i] *= scaleFactor;
            referenceProba[i] *= scaleFactor;
        }
    }

    auto normalizeToUnity = [](absl::Span<double> v) {
        double s = 0;
        for (double x : v)
            s += x;
        for (double& x : v)
            x *= 1.0 / s;
    };
    normalizeToUnity(absl::MakeSpan(proba));
    normalizeToUnity(absl::MakeSpan(referenceProba));

    // compare
    for (size_t i = 0; i < histSize; ++i) {
        double absErr = std::abs(proba[i] - referenceProba[i]);
        if (absErr > maxAbsErr)
            return false;
    }

    return true;
}

TEST_CASE("[Random] Gaussian random generation")
{
    unsigned numGenerations = 16384;
    unsigned numDivisions = 128;

    const double maxAbsErr = 0.05; // PDF ±5%

    REQUIRE(gaussianRandomTest<4>(0.0f, 0.25f, numGenerations, numDivisions, maxAbsErr));
    REQUIRE(gaussianRandomTest<4>(0.0f, 0.50f, numGenerations, numDivisions, maxAbsErr));
    REQUIRE(gaussianRandomTest<4>(0.0f, 0.75f, numGenerations, numDivisions, maxAbsErr));
}
