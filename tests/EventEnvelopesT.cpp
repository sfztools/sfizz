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

#include "sfizz/EventEnvelopes.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/Buffer.h"
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
    std::array<float, 5> expected { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Basic event")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(4, 1.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.25f, 0.5f, 0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, close")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(4, 1.0f);
    envelope.registerEvent(5, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.25f, 0.5f, 0.75f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, far")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.0f, 2.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, reversed")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(6, 2.0f);
    envelope.registerEvent(2, 1.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.0f, 2.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, overlapping")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    envelope.registerEvent(6, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 3.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, out of block")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    envelope.registerEvent(10, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 3 events, out of block, with another block call")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    envelope.registerEvent(10, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };
    envelope.getBlock(absl::MakeSpan(output));
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, with another block call")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f };
    envelope.getBlock(absl::MakeSpan(output));
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] 2 events, function")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.setFunction([](auto x) { return 2 * x; });
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.0f, 4.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized with unquantized targets")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.1f);
    envelope.registerEvent(6, 1.9f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized with 2 steps")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f, 3.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized with 2 steps and an unquantized out of block step")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 3.0f);
    envelope.registerEvent(10, 4.2f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f, 3.0f };
    std::array<float, 8> expected2 { 4.0f, 4.0f, 4.0f,4.0f, 4.0f, 4.0f, 4.0f, 4.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected2);
}


TEST_CASE("[LinearEnvelope] Going down quantized with 2 steps")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.reset(3.0f);
    envelope.registerEvent(2, 2.0f);
    envelope.registerEvent(6, 0.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 3.0f, 2.0f, 2.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized with 2 steps and starting unquantized")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.reset(0.1f);
    envelope.registerEvent(3, 1.0f);
    envelope.registerEvent(7, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.1f, 0.1f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}


TEST_CASE("[LinearEnvelope] Going down quantized with 2 steps and starting unquantized")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.reset(3.6f);
    envelope.registerEvent(4, 1.0f);
    envelope.registerEvent(7, 0.0);
    std::array<float, 8> output;
    std::array<float, 8> expected { 3.6f, 3.0f, 2.0f, 2.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized with unclean events")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.2f);
    envelope.registerEvent(6, 2.5f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f, 3.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[LinearEnvelope] Get quantized 3 events, one out of block")
{
    sfz::LinearEnvelope<float> envelope;
    envelope.registerEvent(2, 1.0f);
    envelope.registerEvent(6, 2.0f);
    envelope.registerEvent(10, 3.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f };
    std::array<float, 8> expected2 { 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected);
    envelope.getQuantizedBlock(absl::MakeSpan(output), 1.0f);
    REQUIRE(output == expected2);
}

//
TEST_CASE("[MultiplicativeEnvelope] Basic state")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    std::array<float, 5> output;
    std::array<float, 5> expected { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[MultiplicativeEnvelope] Basic event")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(4, 2.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.1892f, 1.4142f, 1.68176f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] 2 events")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(4, 2.0f);
    envelope.registerEvent(5, 4.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.1892f, 1.4142f, 1.68176f, 2.0f, 4.0f, 4.0f, 4.0f, 4.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] 2 events, far")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(2, 2.0f);
    envelope.registerEvent(6, 4.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.4142f, 2.0f, 2.37841f, 2.82843f, 3.36358f, 4.0f, 4.0f, 4.0f };
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with 2 steps")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(2, 2.0f);
    envelope.registerEvent(6, 4.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 4.0f, 4.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with an unquantized out of range step")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(2, 2.0f);
    envelope.registerEvent(6, 4.0f);
    envelope.registerEvent(10, 8.2f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 4.0f, 4.0f };
    std::array<float, 8> expected2 { 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected2);
}

TEST_CASE("[MultiplicativeEnvelope] Going down quantized with 2 steps")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.reset(4.0f);
    envelope.registerEvent(2, 2.0f);
    envelope.registerEvent(6, 0.5f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 4.0f, 2.0f, 2.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with unclean events")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.registerEvent(2, 1.2f);
    envelope.registerEvent(6, 2.5f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with 2 steps and starting unquantized")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.reset(0.9f);
    envelope.registerEvent(3, 1.0f);
    envelope.registerEvent(7, 4.0f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.9f, 0.9f, 1.0f, 1.0f, 2.0f, 2.0f, 4.0f, 4.0f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
}


TEST_CASE("[MultiplicativeEnvelope] Going down quantized with 2 steps and starting unquantized")
{
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.reset(4.6f);
    envelope.registerEvent(4, 1.0f);
    envelope.registerEvent(7, 0.25f);
    std::array<float, 8> output;
    std::array<float, 8> expected { 4.6f, 2.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.25f, 0.25f };
    envelope.getQuantizedBlock(absl::MakeSpan(output), 2.0f);
    REQUIRE(output == expected);
}

TEST_CASE("[MultiplicativeEnvelope] Pitch envelope basic function")
{
    sfz::Buffer<float> output { 256 };
    sfz::MultiplicativeEnvelope<float> envelope;
    envelope.setFunction([](float pitchValue){
        const auto normalizedBend = sfz::normalizeBend(pitchValue);
        const auto bendInCents = normalizedBend * 200.0f;
        return sfz::centsFactor(bendInCents);
    });
    envelope.reset(0.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output[255] == 1.0_a);
    envelope.registerEvent(252, -6020.0f);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE(output[255] == Approx(0.9168).epsilon(0.01));
}
