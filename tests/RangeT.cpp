// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Range.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[Range] Equality operators")
{
    Range<int> intRange { 1, 1 };
    REQUIRE(intRange == Range<int>(1, 1));
    REQUIRE(intRange == std::pair<int, int>(1, 1));
    REQUIRE(std::pair<int, int>(1, 1) == intRange);

    Range<float> floatRange { 1.0f, 1.0f };
    REQUIRE(floatRange == Range<float>(1.0f, 1.0f));
    REQUIRE(floatRange == std::pair<float, float>(1.0f, 1.0f));
    REQUIRE(std::pair<float, float>(1.0f, 1.0f) == floatRange);
}

TEST_CASE("[Range] Default ranges for classical types")
{
    Range<int> intRange;
    REQUIRE(intRange == Range<int>(0, 0));
    Range<int32_t> int32_tRange;
    REQUIRE(int32_tRange == Range<int32_t>(0, 0));
    Range<uint32_t> uint32_tRange;
    REQUIRE(uint32_tRange == Range<uint32_t>(0, 0));
    Range<int64_t> int64_tRange;
    REQUIRE(int64_tRange == Range<int64_t>(0, 0));
    Range<uint64_t> uint64_tRange;
    REQUIRE(uint64_tRange == Range<uint64_t>(0, 0));
    Range<float> floatRange;
    REQUIRE(floatRange == Range<float>(0, 0));
    Range<double> doubleRange;
    REQUIRE(doubleRange == Range<double>(0, 0));
}

TEST_CASE("[Range] Contains")
{
    Range<int> intRange { 1, 10 };
    REQUIRE(!intRange.contains(0));
    REQUIRE(intRange.contains(1));
    REQUIRE(intRange.contains(5));
    REQUIRE(!intRange.contains(10));
    REQUIRE(!intRange.containsWithEnd(0));
    REQUIRE(intRange.containsWithEnd(1));
    REQUIRE(intRange.containsWithEnd(5));
    REQUIRE(intRange.containsWithEnd(10));

    Range<float> floatRange { 1.0, 10.0 };
    REQUIRE(!floatRange.contains(0.0));
    REQUIRE(floatRange.contains(1.0));
    REQUIRE(floatRange.contains(5.0));
    REQUIRE(!floatRange.contains(10.0));
    REQUIRE(!floatRange.containsWithEnd(0.0));
    REQUIRE(floatRange.containsWithEnd(1.0));
    REQUIRE(floatRange.containsWithEnd(5.0));
    REQUIRE(floatRange.containsWithEnd(10.0));
}

TEST_CASE("[Range] Clamp")
{
    Range<int> intRange { 1, 10 };
    REQUIRE(intRange.clamp(0) == 1);
    REQUIRE(intRange.clamp(1) == 1);
    REQUIRE(intRange.clamp(5) == 5);
    REQUIRE(intRange.clamp(10) == 10);
    REQUIRE(intRange.clamp(11) == 10);

    Range<float> floatRange { 1.0, 10.0 };
    REQUIRE(floatRange.clamp(0.0) == 1.0_a);
    REQUIRE(floatRange.clamp(1.0) == 1.0_a);
    REQUIRE(floatRange.clamp(5.0) == 5.0_a);
    REQUIRE(floatRange.clamp(10.0) == 10.0_a);
    REQUIRE(floatRange.clamp(11.0) == 10.0_a);
}

TEST_CASE("[Range] shrinkIfSmaller")
{
    Range<int> intRange { 2, 10 };
    intRange.shrinkIfSmaller(0, 10);
    REQUIRE(intRange == Range<int>(2, 10));
    intRange.shrinkIfSmaller(2, 11);
    REQUIRE(intRange == Range<int>(2, 10));
    intRange.shrinkIfSmaller(2, 9);
    REQUIRE(intRange == Range<int>(2, 9));
    intRange.shrinkIfSmaller(3, 9);
    REQUIRE(intRange == Range<int>(3, 9));
    intRange.shrinkIfSmaller(4, 7);
    REQUIRE(intRange == Range<int>(4, 7));
    intRange.shrinkIfSmaller(6, 5);
    REQUIRE(intRange == Range<int>(5, 6));
}