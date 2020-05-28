// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("[SfzHelpers] Normalize values")
{
    REQUIRE(sfz::normalize7Bits(0) == 0.0f);
    REQUIRE(sfz::normalize7Bits(127) == 1.0f);
    REQUIRE(sfz::normalize7Bits(128) == 1.0f);
    REQUIRE(sfz::normalize7Bits(64) == 0.5f);
}

TEST_CASE("[SfzHelpers] Denormalize values")
{
    REQUIRE(sfz::denormalize7Bits<uint8_t>(-3.0f) == 0);
    REQUIRE(sfz::denormalize7Bits<uint8_t>(0.0f) == 0);
    REQUIRE(sfz::denormalize7Bits<uint8_t>(1.0f) == 127);
    REQUIRE(sfz::denormalize7Bits<uint8_t>(1.1f) == 127);
    REQUIRE(sfz::denormalize7Bits<uint8_t>(0.5f) == 64);
}

TEST_CASE("[SfzHelpers] Literals")
{
    REQUIRE(0_norm == 0.0f);
    REQUIRE(127_norm == 1.0f);
    REQUIRE(128_norm == 1.0f);
    REQUIRE(64_norm == 0.5f);
}
