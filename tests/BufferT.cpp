// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Buffer.h"
#include "catch2/catch.hpp"
#include <algorithm>
using namespace Catch::literals;

TEST_CASE("[Buffer] Empty (float)")
{
    sfz::Buffer<float> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (int)")
{
    sfz::Buffer<int> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (double)")
{
    sfz::Buffer<double> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

TEST_CASE("[Buffer] Empty (uint8_t)")
{
    sfz::Buffer<uint8_t> emptyBuffer;
    REQUIRE(emptyBuffer.empty());
    REQUIRE(emptyBuffer.size() == 0);
}

template <class Type>
void checkBoundaries(sfz::Buffer<Type>& buffer, int expectedSize)
{
    REQUIRE((int)buffer.size() == expectedSize);
    REQUIRE(((size_t)buffer.data() & (sfz::SIMDConfig::defaultAlignment - 1)) == 0);
    REQUIRE(((size_t)buffer.alignedEnd() & (sfz::SIMDConfig::defaultAlignment - 1)) == 0);
    REQUIRE(std::distance(buffer.begin(), buffer.end()) == expectedSize);
    REQUIRE(std::distance(buffer.begin(), buffer.alignedEnd()) >= expectedSize);
}

TEST_CASE("[Buffer] 10 floats ")
{
    const int baseSize { 10 };
    sfz::Buffer<float> buffer(baseSize);
    checkBoundaries(buffer, baseSize);

    for (auto& element : buffer)
        element = 0.0f;
    for (auto& element : buffer)
        REQUIRE(element == 0.0f);
}

TEST_CASE("[Buffer] Resize 10 floats ")
{
    const int baseSize { 10 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    sfz::Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    checkBoundaries(buffer, baseSize);

    std::fill(buffer.begin(), buffer.end(), 1.0f);

    REQUIRE(buffer.resize(smallSize));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize));
    checkBoundaries(buffer, bigSize);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Resize 4096 floats ")
{
    const int baseSize { 4096 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    sfz::Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    checkBoundaries(buffer, baseSize);

    std::fill(buffer.begin(), buffer.end(), 1.0f);

    REQUIRE(buffer.resize(smallSize));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize));
    checkBoundaries(buffer, bigSize);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Resize 65536 floats ")
{
    const int baseSize { 10 };
    const int smallSize { baseSize / 2 };
    const int bigSize { baseSize * 2 };
    sfz::Buffer<float> buffer(baseSize);
    REQUIRE(!buffer.empty());
    checkBoundaries(buffer, baseSize);

    std::fill(buffer.begin(), buffer.end(), 1.0f);

    REQUIRE(buffer.resize(smallSize));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize));
    checkBoundaries(buffer, bigSize);
    for (auto i = 0; i < smallSize; ++i)
        REQUIRE(buffer[i] == 1.0f);
}

TEST_CASE("[Buffer] Copy and move")
{
    const int baseSize { 128 };
    sfz::Buffer<float> buffer(baseSize);
    sfz::Buffer<float> copied { baseSize - 4 };
    std::fill(buffer.begin(), buffer.end(), 1.0f);
    std::fill(copied.begin(), copied.end(), 2.0f);
    copied = buffer;
    checkBoundaries(copied, baseSize);
    REQUIRE(std::all_of(copied.begin(), copied.end(), [](float value) { return value == 1.0f; }));

    sfz::Buffer<float> copyConstructed { buffer };
    checkBoundaries(copyConstructed, baseSize);
    REQUIRE(std::all_of(copyConstructed.begin(), copyConstructed.end(), [](float value) { return value == 1.0f; }));

    // sfz::Buffer<float> moveConstructed { std::move(buffer) };
    // REQUIRE(buffer.empty());
    // checkBoundaries(moveConstructed, baseSize);
    // REQUIRE(std::all_of(moveConstructed.begin(), moveConstructed.end(), [](auto value) { return value == 1.0f; }));
}
