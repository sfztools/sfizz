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