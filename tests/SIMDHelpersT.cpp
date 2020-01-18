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

#include "sfizz/SIMDHelpers.h"
#include "catch2/catch.hpp"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <algorithm>
#include <array>
#include <iostream>
using namespace Catch::literals;

constexpr int smallBufferSize { 3 };
constexpr int bigBufferSize { 4095 };
constexpr int medBufferSize { 127 };
constexpr float fillValue { 1.3f };

template <class Type>
inline bool approxEqualMargin(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps = 1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).margin(eps)) {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

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

TEST_CASE("[Helpers] fill() - Manual buffer")
{
    std::vector<float> buffer(5);
    std::vector<float> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    sfz::fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer")
{
    std::vector<float> buffer(smallBufferSize);
    std::vector<float> expected(smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer")
{
    std::vector<float> buffer(bigBufferSize);
    std::vector<float> expected(bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer -- SIMD")
{
    std::vector<float> buffer(smallBufferSize);
    std::vector<float> expected(smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<float, true>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer -- SIMD")
{
    std::vector<float> buffer(bigBufferSize);
    std::vector<float> expected(bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<float, true>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer -- doubles")
{
    std::vector<double> buffer(smallBufferSize);
    std::vector<double> expected(smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<double, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer -- doubles")
{
    std::vector<double> buffer(bigBufferSize);
    std::vector<double> expected(bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    sfz::fill<double, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] Interleaved read")
{
    std::array<float, 16> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f };
    std::array<float, 16> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 8> leftOutput;
    std::array<float, 8> rightOutput;
    sfz::readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 16> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Interleaved read unaligned end")
{
    std::array<float, 20> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    std::array<float, 20> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 10> leftOutput;
    std::array<float, 10> rightOutput;
    sfz::readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 20> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Small interleaved read unaligned end")
{
    std::array<float, 6> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    std::array<float, 6> expected { 0.0f, 1.0f, 2.0f, 10.0f, 11.0f, 12.0f };
    std::array<float, 3> leftOutput;
    std::array<float, 3> rightOutput;
    sfz::readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 6> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Interleaved read -- SIMD")
{
    std::array<float, 16> input = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f };
    std::array<float, 16> expected = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 8> leftOutput;
    std::array<float, 8> rightOutput;
    sfz::readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 16> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Interleaved read unaligned end -- SIMD")
{
    std::array<float, 20> input = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    std::array<float, 20> expected = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 10> leftOutput;
    std::array<float, 10> rightOutput;
    sfz::readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 20> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Small interleaved read unaligned end -- SIMD")
{
    std::array<float, 6> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    std::array<float, 6> expected { 0.0f, 1.0f, 2.0f, 10.0f, 11.0f, 12.0f };
    std::array<float, 3> leftOutput;
    std::array<float, 3> rightOutput;
    sfz::readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 6> real;

    auto realIdx = 0;
    for (auto value : leftOutput)
        real[realIdx++] = value;
    for (auto value : rightOutput)
        real[realIdx++] = value;
    REQUIRE(real == expected);
}

TEST_CASE("[Helpers] Interleaved read SIMD vs Scalar")
{
    std::array<float, medBufferSize * 2> input;
    std::array<float, medBufferSize> leftOutputScalar;
    std::array<float, medBufferSize> rightOutputScalar;
    std::array<float, medBufferSize> leftOutputSIMD;
    std::array<float, medBufferSize> rightOutputSIMD;
    std::iota(input.begin(), input.end(), 0.0f);
    sfz::readInterleaved<float, false>(input, absl::MakeSpan(leftOutputScalar), absl::MakeSpan(rightOutputScalar));
    sfz::readInterleaved<float, true>(input, absl::MakeSpan(leftOutputSIMD), absl::MakeSpan(rightOutputSIMD));
    REQUIRE(leftOutputScalar == leftOutputSIMD);
    REQUIRE(rightOutputScalar == rightOutputSIMD);
}

TEST_CASE("[Helpers] Interleaved write")
{
    std::array<float, 8> leftInput {
        0.0f,
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f,
    };
    std::array<float, 8> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 16> output;
    std::array<float, 16> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f };
    sfz::writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write unaligned end")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    sfz::writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Small interleaved write unaligned end")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f };
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    sfz::writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write -- SIMD")
{
    std::array<float, 8> leftInput {
        0.0f,
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f,
    };
    std::array<float, 8> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 16> output;
    std::array<float, 16> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f };
    sfz::writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write unaligned end -- SIMD")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    sfz::writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Small interleaved write unaligned end -- SIMD")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f };
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    sfz::writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write SIMD vs Scalar")
{
    std::array<float, medBufferSize> leftInput;
    std::array<float, medBufferSize> rightInput;
    std::array<float, medBufferSize * 2> outputScalar;
    std::array<float, medBufferSize * 2> outputSIMD;
    std::iota(leftInput.begin(), leftInput.end(), 0.0f);
    std::iota(rightInput.begin(), rightInput.end(), static_cast<float>(medBufferSize));
    sfz::writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(outputScalar));
    sfz::writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(outputSIMD));
    REQUIRE(outputScalar == outputSIMD);
}

TEST_CASE("[Helpers] Gain, single")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    sfz::applyGain<float, false>(fillValue, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Gain, single and inplace")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    sfz::applyGain<float, false>(fillValue, buffer, absl::MakeSpan(buffer));
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] Gain, spans")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    sfz::applyGain<float, false>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Gain, spans and inplace")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    sfz::applyGain<float, false>(gain, buffer, absl::MakeSpan(buffer));
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] Gain, single (SIMD)")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    sfz::applyGain<float, true>(fillValue, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Gain, single and inplace (SIMD)")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    sfz::applyGain<float, true>(fillValue, buffer, absl::MakeSpan(buffer));
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] Gain, spans (SIMD)")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    sfz::applyGain<float, true>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Gain, spans and inplace (SIMD)")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    sfz::applyGain<float, true>(gain, buffer, absl::MakeSpan(buffer));
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] SFZ looping index")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 1, 2, 4 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 1.0f, 0.5f, 0.9f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 0.0f, 0.5f, 0.1f };
    sfz::loopingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6, 1);
    REQUIRE(indices == expectedIndices);
    REQUIRE(approxEqual<float>(leftCoeffs, expectedLeft));
    REQUIRE(approxEqual<float>(rightCoeffs, expectedRight));
}

TEST_CASE("[Helpers] SFZ looping index (SIMD)")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 1, 2, 4 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 1.0f, 0.5f, 0.9f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 0.0f, 0.5f, 0.1f };
    sfz::loopingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6, 1);
    REQUIRE(indices == expectedIndices);
    REQUIRE(approxEqual<float>(leftCoeffs, expectedLeft));
    REQUIRE(approxEqual<float>(rightCoeffs, expectedRight));
}

// TEST_CASE("[Helpers] SFZ looping index (SIMD vs Scalar)")
// {

//     std::vector<float> jumps(bigBufferSize);
//     absl::c_fill(jumps, fillValue);

//     std::vector<int> indices(bigBufferSize);
//     std::vector<float> leftCoeffs(bigBufferSize);
//     std::vector<float> rightCoeffs(bigBufferSize);

//     std::vector<int> indicesSIMD(bigBufferSize);
//     std::vector<float> leftCoeffsSIMD(bigBufferSize);
//     std::vector<float> rightCoeffsSIMD(bigBufferSize);
//     sfz::loopingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, medBufferSize, 1);
//     sfz::loopingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffsSIMD), absl::MakeSpan(rightCoeffsSIMD), absl::MakeSpan(indicesSIMD), 1.0f, medBufferSize, 1);
//     for (int i = 0; i < bigBufferSize; ++i)
//         REQUIRE( ((static_cast<float>(indices[i]) + rightCoeffs[i] == Approx(static_cast<float>(indicesSIMD[i]) + rightCoeffsSIMD[i]).margin(1e-2))
//                 || (static_cast<float>(indices[i]) + rightCoeffs[i] == Approx(static_cast<float>(indicesSIMD[i]) + rightCoeffsSIMD[i] - static_cast<float>(medBufferSize)).margin(2e-2))) );
// }

TEST_CASE("[Helpers] SFZ saturating index")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 5, 5, 5 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 0.0f, 0.0f, 0.0f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 1.0f, 1.0f, 1.0f };
    sfz::saturatingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6);
    REQUIRE(indices == expectedIndices);
    REQUIRE(approxEqual<float>(leftCoeffs, expectedLeft));
    REQUIRE(approxEqual<float>(rightCoeffs, expectedRight));
}

TEST_CASE("[Helpers] SFZ saturating index (SIMD)")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 5, 5, 5 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 0.0f, 0.0f, 0.0f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 1.0f, 1.0f, 1.0f };
    sfz::saturatingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6);
    REQUIRE(indices == expectedIndices);
    REQUIRE(approxEqualMargin<float>(leftCoeffs, expectedLeft));
    REQUIRE(approxEqualMargin<float>(rightCoeffs, expectedRight));
}

TEST_CASE("[Helpers] SFZ saturating index (SIMD vs Scalar)")
{

    std::vector<float> jumps(medBufferSize);
    absl::c_fill(jumps, fillValue);

    std::vector<int> indices(medBufferSize);
    std::vector<float> leftCoeffs(medBufferSize);
    std::vector<float> rightCoeffs(medBufferSize);

    std::vector<int> indicesSIMD(medBufferSize);
    std::vector<float> leftCoeffsSIMD(medBufferSize);
    std::vector<float> rightCoeffsSIMD(medBufferSize);
    sfz::saturatingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 78);
    sfz::saturatingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffsSIMD), absl::MakeSpan(rightCoeffsSIMD), absl::MakeSpan(indicesSIMD), 1.0f, 78);
    for (int i = 0; i < medBufferSize; ++i)
        REQUIRE( static_cast<float>(indices[i]) + rightCoeffs[i] == Approx(static_cast<float>(indicesSIMD[i]) + rightCoeffsSIMD[i]));
}

TEST_CASE("[Helpers] Linear Ramp")
{
    const float start { 0.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { v, v + v, v + v + v, v + v + v + v, v + v + v + v + v, v + v + v + v + v + v };
    sfz::linearRamp<float, false>(absl::MakeSpan(output), start, v);
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Linear Ramp (SIMD)")
{
    const float start { 0.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { v, v + v, v + v + v, v + v + v + v, v + v + v + v + v, v + v + v + v + v + v };
    sfz::linearRamp<float, true>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Linear Ramp (SIMD vs scalar)")
{
    const float start { 0.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::linearRamp<float, false>(absl::MakeSpan(outputScalar), start, fillValue);
    sfz::linearRamp<float, true>(absl::MakeSpan(outputSIMD), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Linear Ramp unaligned (SIMD vs scalar)")
{
    const float start { 0.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::linearRamp<float, false>(absl::MakeSpan(outputScalar).subspan(1), start, fillValue);
    sfz::linearRamp<float, true>(absl::MakeSpan(outputSIMD).subspan(1), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Multiplicative Ramp")
{
    const float start { 1.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { v, v * v, v * v * v, v * v * v * v, v * v * v * v * v, v * v * v * v * v * v };
    sfz::multiplicativeRamp<float, false>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Multiplicative Ramp (SIMD)")
{
    const float start { 1.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { v, v * v, v * v * v, v * v * v * v, v * v * v * v * v, v * v * v * v * v * v };
    sfz::multiplicativeRamp<float, true>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Multiplicative Ramp (SIMD vs scalar)")
{
    const float start { 1.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::multiplicativeRamp<float, false>(absl::MakeSpan(outputScalar), start, fillValue);
    sfz::multiplicativeRamp<float, true>(absl::MakeSpan(outputSIMD), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Multiplicative Ramp unaligned (SIMD vs scalar)")
{
    const float start { 1.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::multiplicativeRamp<float, false>(absl::MakeSpan(outputScalar).subspan(1), start, fillValue);
    sfz::multiplicativeRamp<float, true>(absl::MakeSpan(outputSIMD).subspan(1), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Add")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    sfz::add<float, false>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Add (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    sfz::add<float, true>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Add (SIMD vs scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(input, 0.0f);
    absl::c_fill(outputScalar, 0.0f);
    absl::c_fill(outputSIMD, 0.0f);

    sfz::add<float, false>(input, absl::MakeSpan(outputScalar));
    sfz::add<float, true>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Subtract")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, -1.0f, -2.0f, -3.0f, -4.0f };
    sfz::subtract<float, false>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Subtract 2")
{
    std::array<float, 5> output { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sfz::subtract<float, false>(1.0f, absl::MakeSpan(output));
    REQUIRE(output == expected);
}


TEST_CASE("[Helpers] Subtract (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, -1.0f, -2.0f, -3.0f, -4.0f };
    sfz::subtract<float, true>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Subtract (SIMD vs scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(input, 0.0f);
    absl::c_fill(outputScalar, 0.0f);
    absl::c_fill(outputSIMD, 0.0f);

    sfz::subtract<float, false>(input, absl::MakeSpan(outputScalar));
    sfz::subtract<float, true>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Subtract 2 (SIMD vs scalar)")
{
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);

    sfz::subtract<float, false>(1.2f, absl::MakeSpan(outputScalar));
    sfz::subtract<float, true>(1.2f, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] copy")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    sfz::copy<float, false>(input, absl::MakeSpan(output));
    REQUIRE(output == input);
}

TEST_CASE("[Helpers] copy (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    sfz::copy<float, true>(input, absl::MakeSpan(output));
    REQUIRE(output == input);
}

TEST_CASE("[Helpers] copy (SIMD vs scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(input, 0.0f);
    absl::c_fill(outputScalar, 0.0f);
    absl::c_fill(outputSIMD, 0.0f);

    sfz::add<float, false>(input, absl::MakeSpan(outputScalar));
    sfz::add<float, true>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Mean")
{
    std::array<float, 10> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    REQUIRE(sfz::mean<float, false>(input) == 5.5f);
    REQUIRE(sfz::mean<float, true>(input) == 5.5f);
}

TEST_CASE("[Helpers] Mean (SIMD vs scalar)")
{
    std::vector<float> input(bigBufferSize);
    absl::c_iota(input, 0.0f);
    REQUIRE(sfz::mean<float, false>(input) == Approx(sfz::mean<float, true>(input)).margin(0.001));
}

TEST_CASE("[Helpers] Mean Squared")
{
    std::array<float, 10> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    REQUIRE(sfz::meanSquared<float, false>(input) == 38.5f);
    REQUIRE(sfz::meanSquared<float, true>(input) == 38.5f);
}

TEST_CASE("[Helpers] Mean Squared (SIMD vs scalar)")
{
    std::vector<float> input(medBufferSize);
    absl::c_iota(input, 0.0f);
    REQUIRE(sfz::meanSquared<float, false>(input) == sfz::meanSquared<float, true>(input));
}

TEST_CASE("[Helpers] Cumulative sum ")
{
    std::array<float, 6> input { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<float, 6> output;
    std::array<float, 6> expected { 1.1f, 2.3f, 3.6f, 5.0f, 6.5f, 8.1f };
    sfz::cumsum<float, false>(input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Cumulative sum (SIMD vs Scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::linearRamp<float>(absl::MakeSpan(input), 0.0f, 0.1f);
    sfz::cumsum<float, false>(input, absl::MakeSpan(outputScalar));
    sfz::cumsum<float, true>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Diff ")
{
    std::array<float, 6> input { 1.1f, 2.3f, 3.6f, 5.0f, 6.5f, 8.1f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f };
    sfz::diff<float, false>(input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Diff (SIMD vs Scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::linearRamp<float>(absl::MakeSpan(input), 0.0f, 0.1f);
    sfz::diff<float, false>(input, absl::MakeSpan(outputScalar));
    sfz::diff<float, true>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}
