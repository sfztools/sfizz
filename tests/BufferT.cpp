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
    REQUIRE(((size_t)buffer.data() & (sfz::config::defaultAlignment - 1)) == 0);
    REQUIRE(((size_t)buffer.alignedEnd() & (sfz::config::defaultAlignment - 1)) == 0);
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

    REQUIRE(buffer.resize(smallSize, std::nothrow));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize, std::nothrow));
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

    REQUIRE(buffer.resize(smallSize, std::nothrow));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize, std::nothrow));
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

    REQUIRE(buffer.resize(smallSize, std::nothrow));
    checkBoundaries(buffer, smallSize);

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](float value) { return value == 1.0f; }));

    REQUIRE(buffer.resize(bigSize, std::nothrow));
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

    sfz::Buffer<float> moveConstructed { std::move(buffer) };
    REQUIRE(buffer.empty());
    checkBoundaries(moveConstructed, baseSize);
    REQUIRE(std::all_of(moveConstructed.begin(), moveConstructed.end(), [](float value) { return value == 1.0f; }));
}

TEST_CASE("[Buffer] Buffer counter")
{
    sfz::BufferCounter& counter = sfz::Buffer<float>::counter();

    // handle the eventuality that the buffer counter does not start at zero
    const size_t initialNumBuffers = counter.getNumBuffers();
    const size_t initialTotalBytes = counter.getTotalBytes();
    auto haveNumBuffers = [&](size_t n) -> bool {
        return n == counter.getNumBuffers() - initialNumBuffers;
    };
    auto haveTotalAllocation = [&](size_t n) -> bool {
        return n * sizeof(float) == counter.getTotalBytes() - initialTotalBytes;
    };

    // create an empty buffer
    sfz::Buffer<float> b1;
    REQUIRE(haveNumBuffers(0));
    REQUIRE(haveTotalAllocation(0));

    // clear an empty buffer
    b1.clear();
    REQUIRE(haveNumBuffers(0));
    REQUIRE(haveTotalAllocation(0));

    // create a sized buffer
    sfz::Buffer<float> b2(5);
    REQUIRE(haveNumBuffers(1));
    REQUIRE(haveTotalAllocation(b2.allocationSize()));

    // resize an empty buffer
    b1.resize(3);
    REQUIRE(haveNumBuffers(2));
    REQUIRE(haveTotalAllocation(b1.allocationSize() + b2.allocationSize()));

    // resize a non-empty buffer
    b1.resize(7);
    REQUIRE(haveNumBuffers(2));
    REQUIRE(haveTotalAllocation(b1.allocationSize() + b2.allocationSize()));

    // clear a non-empty buffer
    b2.clear();
    REQUIRE(haveNumBuffers(1));
    REQUIRE(haveTotalAllocation(b1.allocationSize()));

    // move an empty buffer into a non-empty one
    b1 = std::move(b2);
    REQUIRE(haveNumBuffers(0));
    REQUIRE(haveTotalAllocation(0));

    // move a non-empty buffer into an empty one
    b1.resize(3);
    b2 = std::move(b1);
    REQUIRE(haveNumBuffers(1));
    REQUIRE(haveTotalAllocation(b2.allocationSize()));
}
