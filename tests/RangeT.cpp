// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Range.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[Range] Equality operators")
{
    sfz::Range<int> intRange { 1, 1 };
    REQUIRE(intRange == sfz::Range<int>(1, 1));

    sfz::Range<float> floatRange { 1.0f, 1.0f };
    REQUIRE(floatRange == sfz::Range<float>(1.0f, 1.0f));
}

TEST_CASE("[Range] Default ranges for classical types")
{
    sfz::Range<int> intRange;
    REQUIRE(intRange == sfz::Range<int>(0, 0));
    sfz::Range<int32_t> int32_tRange;
    REQUIRE(int32_tRange == sfz::Range<int32_t>(0, 0));
    sfz::Range<uint32_t> uint32_tRange;
    REQUIRE(uint32_tRange == sfz::Range<uint32_t>(0, 0));
    sfz::Range<int64_t> int64_tRange;
    REQUIRE(int64_tRange == sfz::Range<int64_t>(0, 0));
    sfz::Range<uint64_t> uint64_tRange;
    REQUIRE(uint64_tRange == sfz::Range<uint64_t>(0, 0));
    sfz::Range<float> floatRange;
    REQUIRE(floatRange == sfz::Range<float>(0, 0));
    sfz::Range<double> doubleRange;
    REQUIRE(doubleRange == sfz::Range<double>(0, 0));
}

TEST_CASE("[Range] Contains")
{
    sfz::Range<int> intRange { 1, 10 };
    REQUIRE(!intRange.contains(0));
    REQUIRE(intRange.contains(1));
    REQUIRE(intRange.contains(5));
    REQUIRE(!intRange.contains(10));
    REQUIRE(!intRange.containsWithEnd(0));
    REQUIRE(intRange.containsWithEnd(1));
    REQUIRE(intRange.containsWithEnd(5));
    REQUIRE(intRange.containsWithEnd(10));

    sfz::Range<float> floatRange { 1.0, 10.0 };
    REQUIRE(!floatRange.contains(0.0));
    REQUIRE(floatRange.contains(1.0));
    REQUIRE(floatRange.contains(5.0));
    REQUIRE(!floatRange.contains(10.0));
    REQUIRE(!floatRange.containsWithEnd(0.0));
    REQUIRE(floatRange.containsWithEnd(1.0));
    REQUIRE(floatRange.containsWithEnd(5.0));
    REQUIRE(floatRange.containsWithEnd(10.0));
}

TEST_CASE("[Range] Unchecked ranges")
{
    sfz::UncheckedRange<int> intRange { 10, 1 };
    REQUIRE(intRange.getStart() == 10);
    REQUIRE(intRange.getEnd() == 1);
    REQUIRE(!intRange.isValid());
    for (int v : {0, 1, 5, 10}) {
        REQUIRE(!intRange.contains(v));
        REQUIRE(!intRange.containsWithEnd(v));
    }

    sfz::UncheckedRange<float> floatRange { 10.0, 1.0 };
    REQUIRE(floatRange.getStart() == 10.0f);
    REQUIRE(floatRange.getEnd() == 1.0f);
    REQUIRE(!floatRange.isValid());
    for (float v : {0.0f, 1.0f, 5.0f, 10.0f}) {
        REQUIRE(!floatRange.contains(v));
        REQUIRE(!floatRange.containsWithEnd(v));
    }

    REQUIRE(sfz::UncheckedRange<int> { 1, 10 }.isValid());
    REQUIRE(sfz::UncheckedRange<int> { 1, 1 }.isValid());
    REQUIRE(sfz::UncheckedRange<float> { 1.0, 10.0 }.isValid());
    REQUIRE(sfz::UncheckedRange<float> { 10.0, 10.0 }.isValid());
}

TEST_CASE("[Range] Clamp")
{
    sfz::Range<int> intRange { 1, 10 };
    REQUIRE(intRange.clamp(0) == 1);
    REQUIRE(intRange.clamp(1) == 1);
    REQUIRE(intRange.clamp(5) == 5);
    REQUIRE(intRange.clamp(10) == 10);
    REQUIRE(intRange.clamp(11) == 10);

    sfz::Range<float> floatRange { 1.0, 10.0 };
    REQUIRE(floatRange.clamp(0.0) == 1.0_a);
    REQUIRE(floatRange.clamp(1.0) == 1.0_a);
    REQUIRE(floatRange.clamp(5.0) == 5.0_a);
    REQUIRE(floatRange.clamp(10.0) == 10.0_a);
    REQUIRE(floatRange.clamp(11.0) == 10.0_a);
}

TEST_CASE("[Range] shrinkIfSmaller")
{
    sfz::Range<int> intRange { 2, 10 };
    intRange.shrinkIfSmaller(0, 10);
    REQUIRE(intRange == sfz::Range<int>(2, 10));
    intRange.shrinkIfSmaller(2, 11);
    REQUIRE(intRange == sfz::Range<int>(2, 10));
    intRange.shrinkIfSmaller(2, 9);
    REQUIRE(intRange == sfz::Range<int>(2, 9));
    intRange.shrinkIfSmaller(3, 9);
    REQUIRE(intRange == sfz::Range<int>(3, 9));
    intRange.shrinkIfSmaller(4, 7);
    REQUIRE(intRange == sfz::Range<int>(4, 7));
    intRange.shrinkIfSmaller(6, 5);
    REQUIRE(intRange == sfz::Range<int>(5, 6));
}

TEST_CASE("[Range] expandTo")
{
    sfz::Range<int> intRange { 2, 10 };
    intRange.expandTo(5);
    REQUIRE(intRange == sfz::Range<int>(2, 10));
    intRange.expandTo(10);
    REQUIRE(intRange == sfz::Range<int>(2, 10));
    intRange.expandTo(2);
    REQUIRE(intRange == sfz::Range<int>(2, 10));
    intRange.expandTo(1);
    REQUIRE(intRange == sfz::Range<int>(1, 10));
    intRange.expandTo(-10);
    REQUIRE(intRange == sfz::Range<int>(-10, 10));
    intRange.expandTo(12);
    REQUIRE(intRange == sfz::Range<int>(-10, 12));
    intRange.expandTo(6);
    REQUIRE(intRange == sfz::Range<int>(-10, 12));
}
