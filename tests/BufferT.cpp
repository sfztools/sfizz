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

template<class Type>
void checkBoundaries(Buffer<Type>& buffer, size_t expectedSize)
{
    REQUIRE(buffer.size() == expectedSize);
    REQUIRE(((size_t)buffer.data() & (config::defaultAlignment - 1)) == 0);
    REQUIRE(((size_t)buffer.alignedEnd() & (config::defaultAlignment - 1)) == 0);
    REQUIRE(std::distance(buffer.begin(), buffer.end()) == expectedSize);
    REQUIRE(std::distance(buffer.begin(), buffer.alignedEnd()) >= expectedSize);
}

TEST_CASE("[Buffer] 10 floats ")
{
    const int baseSize { 10 };
    Buffer<float> buffer(baseSize);
    checkBoundaries(buffer, baseSize);
    
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
    checkBoundaries(buffer, baseSize);
    
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    
    REQUIRE( buffer.resize(smallSize) );
    checkBoundaries(buffer, smallSize);

    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    
    REQUIRE( buffer.resize(bigSize) );
    checkBoundaries(buffer, bigSize);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Resize 4096 floats ")
{
    const int baseSize { 4096 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    checkBoundaries(buffer, baseSize);
    
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    
    REQUIRE( buffer.resize(smallSize) );
    checkBoundaries(buffer, smallSize);
    
    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    
    REQUIRE( buffer.resize(bigSize) );
    checkBoundaries(buffer, bigSize);
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
    checkBoundaries(buffer, baseSize);
    
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    
    REQUIRE( buffer.resize(smallSize) );
    checkBoundaries(buffer, smallSize);
    
    REQUIRE( std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }) );
    
    REQUIRE( buffer.resize(bigSize) );
    checkBoundaries(buffer, bigSize);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}