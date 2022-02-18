// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include "sfizz/MathHelpers.h"
#include <cmath>
#include <limits>

TEST_CASE("[FloatMath] Fast ilog2 (float)")
{
    for (float x = -100.0f; x < +100.0f; x += 0.01f) {
        int ex1 = fp_exponent(x);
        int ex2 = static_cast<int>(std::floor(std::log2(std::fabs(x))));
        REQUIRE(ex1 == ex2);
    }
}

TEST_CASE("[FloatMath] Fast ilog2 (double)")
{
    for (double x = -100.0; x < +100.0; x += 0.01) {
        int ex1 = fp_exponent(x);
        int ex2 = static_cast<int>(std::floor(std::log2(std::fabs(x))));
        REQUIRE(ex1 == ex2);
    }
}

TEST_CASE("[FloatMath] Break apart and reconstruct (float)")
{
    for (int p = 0; p < 128; ++p) {
        float f = 440.0 * std::pow(2.0, (p - 69.0) / 12.0);

        bool sgn = fp_sign(f);
        int ex = fp_exponent(f);
        Fraction<uint64_t> mant = fp_mantissa(f);

        REQUIRE(fp_from_parts<float>(sgn, ex, mant.num) == f);
    }
}

TEST_CASE("[FloatMath] Break apart and reconstruct (double)")
{
    for (int p = 0; p < 128; ++p) {
        double f = 440.0 * std::pow(2.0, (p - 69.0) / 12.0);

        bool sgn = fp_sign(f);
        int ex = fp_exponent(f);
        Fraction<uint64_t> mant = fp_mantissa(f);

        REQUIRE(fp_from_parts<double>(sgn, ex, mant.num) == f);
    }
}

TEST_CASE("[FloatMath] Nan/Inf checker")
{
    REQUIRE(fp_naninf(std::numeric_limits<double>::quiet_NaN()));
    REQUIRE(fp_naninf(std::numeric_limits<float>::quiet_NaN()));
    REQUIRE(fp_naninf(std::numeric_limits<double>::infinity()));
    REQUIRE(fp_naninf(std::numeric_limits<float>::infinity()));
    REQUIRE(fp_naninf(-std::numeric_limits<double>::infinity()));
    REQUIRE(fp_naninf(-std::numeric_limits<float>::infinity()));
    REQUIRE(!fp_naninf(0.0f));
    REQUIRE(!fp_naninf(0.0));
    REQUIRE(!fp_naninf(1.0f));
    REQUIRE(!fp_naninf(1.0));
    REQUIRE(!fp_naninf(-1.0f));
    REQUIRE(!fp_naninf(-1.0));
}
