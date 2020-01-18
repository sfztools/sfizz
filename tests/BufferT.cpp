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

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }));

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

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }));

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

    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [](auto value) { return value == 1.0f; }));

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
    REQUIRE(std::all_of(copied.begin(), copied.end(), [](auto value) { return value == 1.0f; }));

    sfz::Buffer<float> copyConstructed { buffer };
    checkBoundaries(copyConstructed, baseSize);
    REQUIRE(std::all_of(copyConstructed.begin(), copyConstructed.end(), [](auto value) { return value == 1.0f; }));

    // sfz::Buffer<float> moveConstructed { std::move(buffer) };
    // REQUIRE(buffer.empty());
    // checkBoundaries(moveConstructed, baseSize);
    // REQUIRE(std::all_of(moveConstructed.begin(), moveConstructed.end(), [](auto value) { return value == 1.0f; }));
}
