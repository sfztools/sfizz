#include "catch2/catch.hpp"
#include "../sources/Buffer.h"
#include <algorithm>
using namespace Catch::literals;

TEST_CASE("[Buffer] Empty (float)")
{
    Buffer<float> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (int)")
{
    Buffer<int> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (double)")
{
    Buffer<double> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (uint8_t)")
{
    Buffer<uint8_t> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] 10 floats ")
{
    Buffer<float> buffer(10);
    REQUIRE(!buffer.empty());
    REQUIRE(buffer.size() == 10);
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    for (auto& element: buffer)
        element = 0.0f;
    for (auto& element: buffer)
        REQUIRE(element == 0.0f);
}

TEST_CASE("[Buffer] Resize 10 floats ")
{
    const int baseSize { 10 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    REQUIRE(buffer.size() == baseSize);
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    REQUIRE( buffer.resize(smallSize) );
    REQUIRE( buffer.size() == smallSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    REQUIRE( buffer.resize(bigSize) );
    REQUIRE( buffer.size() == bigSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Resize 4096 floats ")
{
    const int baseSize { 10 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    REQUIRE(buffer.size() == baseSize);
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    REQUIRE( buffer.resize(smallSize) );
    REQUIRE( buffer.size() == smallSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    REQUIRE( buffer.resize(bigSize) );
    REQUIRE( buffer.size() == bigSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Resize 65536 floats ")
{
    const int baseSize { 10 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    REQUIRE(buffer.size() == baseSize);
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    REQUIRE( buffer.resize(smallSize) );
    REQUIRE( buffer.size() == smallSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    REQUIRE( buffer.resize(bigSize) );
    REQUIRE( buffer.size() == bigSize );
    REQUIRE(((size_t)buffer.data() & (sfz::Config::defaultAlignment - 1)) == 0);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}