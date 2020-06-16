// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include "sfizz/MathHelpers.h"
#include <vector>

/**
 * @brief Check the behavior of a uniform real random generator
 *
 * - ensure all results to be in range [min;max]
 * - ensure to have at least one element in every division of the result range
 */
template <class Rand, class Dist>
static bool randomTest(float min, float max, unsigned gen, unsigned div)
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

    auto& test = randomTest<fast_rand, fast_real_distribution<float>>;

    REQUIRE(test(0.0f, 1.0f, numGenerations, numDivisions));
    REQUIRE(test(-1.0f, 1.0f, numGenerations, numDivisions));
    REQUIRE(test(0.0f, 123.0f, numGenerations, numDivisions));
    REQUIRE(test(-123.0f, 0.0f, numGenerations, numDivisions));
}
