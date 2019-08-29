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

#include "../sources/LinearEnvelope.h"
#include "catch2/catch.hpp"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <algorithm>
#include <array>
#include <iostream>
using namespace Catch::literals;

template <class Type>
inline bool approxEqual(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps = 1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).epsilon(eps)) {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

TEST_CASE("[LinearEnvelope] Basic state")
{
    sfz::LinearEnvelope<float> envelope;
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.0, 0.0, 0.0, 0.0, 0.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Basic event")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(4, 1.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.25, 0.5, 0.75, 1.0, 1.0, 1.0, 1.0, 1.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, close")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(4, 1.0);
    envelope.registerEvent(5, 2.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.25, 0.5, 0.75, 1.0, 2.0, 2.0, 2.0, 2.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, far")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5, 1, 1.25, 1.5, 1.75, 2.0, 2.0, 2.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, reversed")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(6, 2.0);
    envelope.registerEvent(2, 1.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5, 1, 1.25, 1.5, 1.75, 2.0, 2.0, 2.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, overlapping")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    envelope.registerEvent(6, 3.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5, 1, 1.25, 1.5, 1.75, 2.0, 3.0, 3.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, out of block")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    envelope.registerEvent(10, 3.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5, 1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, out of block, with another block call")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    envelope.registerEvent(10, 3.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0 };
    envelope.getBlock(absl::MakeSpan(output));
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, with another block call")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0 };
    envelope.getBlock(absl::MakeSpan(output));
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, function")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.setFunction([](auto x) { return 2 * x; });
    envelope.registerEvent(2, 1.0);
    envelope.registerEvent(6, 2.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1, 2, 2.5, 3, 3.5, 4.0, 4.0, 4.0 };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}