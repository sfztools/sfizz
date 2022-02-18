// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/simd/Common.h"
#include "sfizz/SIMDHelpers.h"
#include "sfizz/Panning.h"
#include "catch2/catch.hpp"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <jsl/allocator>
using namespace Catch::literals;

template <class T, std::size_t A = sfz::config::defaultAlignment>
using aligned_vector = std::vector<T, jsl::aligned_allocator<T, A>>;

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

TEST_CASE("[Helpers] willAlign, prevAligned and unaligned tests")
{
    aligned_vector<float, 32> array(16);
    REQUIRE( !unaligned<16>(&array[0]) );
    REQUIRE( !unaligned<16>(&array[4]) );
    REQUIRE( !unaligned<32>(&array[8]) );
    REQUIRE( unaligned<32>(&array[7]) );
    REQUIRE( unaligned<32>(&array[4]) );
    REQUIRE( unaligned<16>(&array[3]) );
    REQUIRE( !unaligned<16>(&array[0], &array[4]) );
    REQUIRE( !unaligned<16>(&array[0], &array[4], &array[8]) );
    REQUIRE( unaligned<16>(&array[0], &array[3], &array[8]) );

    REQUIRE( prevAligned<16>(&array[0]) == &array[0] );
    REQUIRE( prevAligned<16>(&array[1]) == &array[0] );
    REQUIRE( prevAligned<16>(&array[2]) == &array[0] );
    REQUIRE( prevAligned<16>(&array[3]) == &array[0] );
    REQUIRE( prevAligned<16>(&array[4]) == &array[4] );
    REQUIRE( prevAligned<16>(&array[5]) == &array[4] );
    REQUIRE( prevAligned<32>(&array[7]) == &array[0] );
    REQUIRE( prevAligned<32>(&array[8]) == &array[8] );
    REQUIRE( prevAligned<32>(&array[9]) == &array[8] );

    REQUIRE( willAlign<16>(&array[0], &array[4]) );
    REQUIRE( willAlign<16>(&array[5], &array[1]) );
    REQUIRE( !willAlign<16>(&array[2], &array[1]) );
    REQUIRE( willAlign<32>(&array[9], &array[1]) );
    REQUIRE( willAlign<32>(&array[8], &array[0]) );

    float* meanPointer = (float*)((uint8_t*)&array[1] + 1);
    REQUIRE( !willAlign<16>(&array[0], meanPointer) );
    REQUIRE( !willAlign<16>(&array[4], &array[0], meanPointer) );
}

TEST_CASE("[Helpers] Interleaved read")
{
    std::array<float, 16> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f };
    std::array<float, 16> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 8> leftOutput;
    std::array<float, 8> rightOutput;
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutputScalar), absl::MakeSpan(rightOutputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(input, absl::MakeSpan(leftOutputSIMD), absl::MakeSpan(rightOutputSIMD));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write unaligned end")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Small interleaved write unaligned end")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f };
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Interleaved write unaligned end -- SIMD")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f };
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Small interleaved write unaligned end -- SIMD")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f };
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(output));
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
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(leftInput, rightInput, absl::MakeSpan(outputSIMD));
    REQUIRE(outputScalar == outputSIMD);
}

TEST_CASE("[Helpers] Gain, single")
{
    std::array<float, 65> input;
    std::array<float, 65> expected;
    absl::c_fill(input, 1.0f);
    absl::c_fill(expected, fillValue);

    SECTION("Scalar")
    {
        std::array<float, 65> output;
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, false);
        sfz::applyGain1<float>(fillValue, input, absl::MakeSpan(output));
        REQUIRE(output == expected);
    }

    SECTION("SIMD")
    {
        std::array<float, 65> output;
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, true);
        sfz::applyGain1<float>(fillValue, input, absl::MakeSpan(output));
        REQUIRE(output == expected);
    }
}

TEST_CASE("[Helpers] Gain, single and inplace")
{
    std::array<float, 65> expected;
    std::array<float, 65> buffer;
    absl::c_fill(expected, fillValue);
    SECTION("Scalar")
    {
        absl::c_fill(buffer, 1.0f);
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, false);
        sfz::applyGain1<float>(fillValue, buffer, absl::MakeSpan(buffer));
        REQUIRE(buffer == expected);
    }
    SECTION("SIMD")
    {
        absl::c_fill(buffer, 1.0f);
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, false);
        sfz::applyGain1<float>(fillValue, buffer, absl::MakeSpan(buffer));
        REQUIRE(buffer == expected);
    }
}

TEST_CASE("[Helpers] Gain, spans")
{
    std::array<float, 65> input;
    std::array<float, 65> gain;
    std::array<float, 65> expected;
    absl::c_fill(input, 1.0f);
    absl::c_iota(gain, 1.0f);
    absl::c_iota(expected, 1.0f);

    SECTION("Scalar")
    {
        std::array<float, 65> output;
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, false);
        sfz::applyGain<float>(gain, input, absl::MakeSpan(output));
        REQUIRE(output == expected);
    }

    SECTION("SIMD")
    {
        std::array<float, 65> output;
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, true);
        sfz::applyGain<float>(gain, input, absl::MakeSpan(output));
        REQUIRE(output == expected);
    }
}

TEST_CASE("[Helpers] Gain, spans and inplace")
{
    std::array<float, 65> buffer;
    std::array<float, 65> gain;
    std::array<float, 65> expected;
    absl::c_iota(gain, 1.0f);
    absl::c_iota(expected, 1.0f);

    SECTION("Scalar")
    {
        absl::c_fill(buffer, 1.0f);
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, false);
        sfz::applyGain<float>(gain, buffer, absl::MakeSpan(buffer));
        REQUIRE(buffer == expected);
    }

    SECTION("SIMD")
    {
        absl::c_fill(buffer, 1.0f);
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, false);
        sfz::applyGain<float>(gain, buffer, absl::MakeSpan(buffer));
        REQUIRE(buffer == expected);
    }
}

TEST_CASE("[Helpers] Linear Ramp")
{
    const float start { 0.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { start, start + v, start + v + v, start + v + v + v, start + v + v + v + v, start + v + v + v + v + v };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, false);
    sfz::linearRamp<float>(absl::MakeSpan(output), start, v);
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Linear Ramp (SIMD)")
{
    const float start { 0.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { start, start + v, start + v + v, start + v + v + v, start + v + v + v + v, start + v + v + v + v + v };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, true);
    sfz::linearRamp<float>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Linear Ramp (SIMD vs scalar)")
{
    const float start { 0.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, false);
    sfz::linearRamp<float>(absl::MakeSpan(outputScalar), start, fillValue);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, true);
    sfz::linearRamp<float>(absl::MakeSpan(outputSIMD), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Linear Ramp unaligned (SIMD vs scalar)")
{
    const float start { 0.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, false);
    sfz::linearRamp<float>(absl::MakeSpan(outputScalar).subspan(1), start, fillValue);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, true);
    sfz::linearRamp<float>(absl::MakeSpan(outputSIMD).subspan(1), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Multiplicative Ramp")
{
    const float start { 1.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { start, start * v, start * v * v, start * v * v * v, start * v * v * v * v, start * v * v * v * v * v };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, false);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Multiplicative Ramp (SIMD)")
{
    const float start { 1.0f };
    const float v { fillValue };
    std::array<float, 6> output;
    std::array<float, 6> expected { start, start * v, start * v * v, start * v * v * v, start * v * v * v * v, start * v * v * v * v * v };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, true);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(output), start, v);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Multiplicative Ramp (SIMD vs scalar)")
{
    const float start { 1.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, false);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(outputScalar), start, fillValue);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, true);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(outputSIMD), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Multiplicative Ramp unaligned (SIMD vs scalar)")
{
    const float start { 1.0f };
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, false);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(outputScalar).subspan(1), start, fillValue);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplicativeRamp, true);
    sfz::multiplicativeRamp<float>(absl::MakeSpan(outputSIMD).subspan(1), start, fillValue);
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Add")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, false);
    sfz::add<float>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Add (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, true);
    sfz::add<float>(input, absl::MakeSpan(output));
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

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, false);
    sfz::add<float>(input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, true);
    sfz::add<float>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] MultiplyAdd (Scalar)")
{
    std::array<float, 5> gain { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 5.0f, 4.2f, 3.6f, 3.2f, 3.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, false);
    sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] MultiplyAdd (SIMD)")
{
    std::array<float, 5> gain { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 5.0f, 4.2f, 3.6f, 3.2f, 3.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, true);
    sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}


TEST_CASE("[Helpers] MultiplyAdd (SIMD vs scalar)")
{
    std::vector<float> gain(bigBufferSize);
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(gain, 0.0f);
    absl::c_iota(input, 0.0f);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, false);
    sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, true);
    sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] MultiplyAdd fixed gain (Scalar)")
{
    float gain = 0.3f;
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 5.3f, 4.6f, 3.9f, 3.2f, 2.5f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, false);
    sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] MultiplyAdd fixed gain (SIMD)")
{
    float gain = 0.3f;
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 5.3f, 4.6f, 3.9f, 3.2f, 2.5f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, true);
    sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] MultiplyAdd fixed gain (SIMD vs scalar)")
{
    float gain = 0.3f;
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(input, 0.0f);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, false);
    sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, true);
    sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] MultiplyMul (Scalar)")
{
    std::array<float, 5> gain { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, 0.8f, 1.8f, 2.4f, 2.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul, true);
    sfz::multiplyMul<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] MultiplyMul (SIMD)")
{
    std::array<float, 5> gain { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, 0.8f, 1.8f, 2.4f, 2.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul, true);
    sfz::multiplyMul<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] MultiplyMul (SIMD vs Scalar)")
{
    std::vector<float> gain(bigBufferSize);
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(gain, 0.0f);
    absl::c_iota(input, 0.0f);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul, false);
    sfz::multiplyMul<float>(gain, input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul, true);
    sfz::multiplyMul<float>(gain, input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] MultiplyMul fixed gain (Scalar)")
{
    float gain = 0.3f;
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 1.5f, 2.4f, 2.7f, 2.4f, 1.5f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, false);
    sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] MultiplyMul fixed gain (SIMD)")
{
    float gain = 0.3f;
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::array<float, 5> expected { 1.5f, 2.4f, 2.7f, 2.4f, 1.5f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, true);
    sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] MultiplyMul fixed gain (SIMD vs scalar)")
{
    float gain = 0.3f;
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(input, 0.0f);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, false);
    sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, true);
    sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Subtract")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, -1.0f, -2.0f, -3.0f, -4.0f };
    sfz::subtract<float>(input, absl::MakeSpan(output));
    REQUIRE(output == expected);
}

TEST_CASE("[Helpers] Subtract 2")
{
    std::array<float, 5> output { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract1, false);
    sfz::subtract1<float>(1.0f, absl::MakeSpan(output));
    REQUIRE(output == expected);
}


TEST_CASE("[Helpers] Subtract (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { 0.0f, -1.0f, -2.0f, -3.0f, -4.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract, true);
    sfz::subtract<float>(input, absl::MakeSpan(output));
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

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract, false);
    sfz::subtract<float>(input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract, true);
    sfz::subtract<float>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Subtract 2 (SIMD vs scalar)")
{
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    absl::c_iota(outputScalar, 0.0f);
    absl::c_iota(outputSIMD, 0.0f);

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract1, false);
    sfz::subtract1<float>(1.2f, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::subtract1, true);
    sfz::subtract1<float>(1.2f, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] copy")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, false);
    sfz::copy<float>(input, absl::MakeSpan(output));
    REQUIRE(output == input);
}

TEST_CASE("[Helpers] copy (SIMD)")
{
    std::array<float, 5> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, true);
    sfz::copy<float>(input, absl::MakeSpan(output));
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

    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, false);
    sfz::copy<float>(input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, true);
    sfz::copy<float>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Mean")
{
    std::array<float, 10> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::mean, false);
    REQUIRE(sfz::mean<float>(input) == 5.5f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::mean, true);
    REQUIRE(sfz::mean<float>(input) == 5.5f);
}

TEST_CASE("[Helpers] Mean (SIMD vs scalar)")
{
    std::vector<float> input(bigBufferSize);
    absl::c_iota(input, 0.0f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::mean, false);
    auto scalarResult = sfz::mean<float>(input);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::mean, true);
    auto simdResult = sfz::mean<float>(input);
    REQUIRE( scalarResult == Approx(simdResult).margin(1e-3) );
}

TEST_CASE("[Helpers] Mean Squared")
{
    std::array<float, 10> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, false);
    REQUIRE(sfz::meanSquared<float>(input) == 38.5f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, true);
    REQUIRE(sfz::meanSquared<float>(input) == 38.5f);
}

TEST_CASE("[Helpers] Mean Squared (SIMD vs scalar)")
{
    std::vector<float> input(medBufferSize);
    absl::c_iota(input, 0.0f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, false);
    auto scalarResult = sfz::meanSquared<float>(input);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, true);
    auto simdResult = sfz::meanSquared<float>(input);
    REQUIRE( scalarResult == Approx(simdResult).margin(1e-3) );
}

TEST_CASE("[Helpers] Cumulative sum")
{
    std::array<float, 6> input { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f }; // 1.1 2.3 3.6 5.0f 6.5 8.1
    std::array<float, 6> output;
    std::array<float, 6> expected { 1.1f, 2.3f, 3.6f, 5.0f, 6.5f, 8.1f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::cumsum, false);
    sfz::cumsum<float>(input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Cumulative sum (SIMD vs Scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, true);
    sfz::linearRamp<float>(absl::MakeSpan(input), 0.0f, 0.1f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::cumsum, false);
    sfz::cumsum<float>(input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::cumsum, true);
    sfz::cumsum<float>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

TEST_CASE("[Helpers] Diff")
{
    std::array<float, 6> input { 1.1f, 2.3f, 3.6f, 5.0f, 6.5f, 8.1f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::diff, false);
    sfz::diff<float>(input, absl::MakeSpan(output));
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[Helpers] Diff (SIMD vs Scalar)")
{
    std::vector<float> input(bigBufferSize);
    std::vector<float> outputScalar(bigBufferSize);
    std::vector<float> outputSIMD(bigBufferSize);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::linearRamp, true);
    sfz::linearRamp<float>(absl::MakeSpan(input), 0.0f, 0.1f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::diff, false);
    sfz::diff<float>(input, absl::MakeSpan(outputScalar));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::diff, true);
    sfz::diff<float>(input, absl::MakeSpan(outputSIMD));
    REQUIRE(approxEqual<float>(outputScalar, outputSIMD));
}

template<unsigned N>
void panTest(float leftValue, float rightValue, float panValue, float expectedLeft, float expectedRight)
{
    std::vector<float> leftChannel(N);
    std::vector<float> rightChannel(N);
    std::vector<float> pan(N);
    std::vector<float> expectedLeftChannel(N);
    std::vector<float> expectedRightChannel(N);
    std::fill(leftChannel.begin(), leftChannel.end(), leftValue);
    std::fill(expectedLeftChannel.begin(), expectedLeftChannel.end(), expectedLeft);
    std::fill(rightChannel.begin(), rightChannel.end(), rightValue);
    std::fill(expectedRightChannel.begin(), expectedRightChannel.end(), expectedRight);
    std::fill(pan.begin(), pan.end(), panValue);
    auto left = absl::MakeSpan(leftChannel);
    auto right = absl::MakeSpan(rightChannel);
    sfz::pan(pan, left, right);
    REQUIRE_THAT( leftChannel, Catch::Approx(expectedLeftChannel).margin(0.001) );
    REQUIRE_THAT( rightChannel, Catch::Approx(expectedRightChannel).margin(0.001) );
}

template<unsigned N>
void widthTest(float leftValue, float rightValue, float widthValue, float expectedLeft, float expectedRight)
{
    std::vector<float> leftChannel(N);
    std::vector<float> rightChannel(N);
    std::vector<float> width(N);
    std::vector<float> expectedLeftChannel(N);
    std::vector<float> expectedRightChannel(N);
    std::fill(leftChannel.begin(), leftChannel.end(), leftValue);
    std::fill(expectedLeftChannel.begin(), expectedLeftChannel.end(), expectedLeft);
    std::fill(rightChannel.begin(), rightChannel.end(), rightValue);
    std::fill(expectedRightChannel.begin(), expectedRightChannel.end(), expectedRight);
    std::fill(width.begin(), width.end(), widthValue);
    auto left = absl::MakeSpan(leftChannel);
    auto right = absl::MakeSpan(rightChannel);
    sfz::width(width, left, right);
    REQUIRE_THAT( leftChannel, Catch::Approx(expectedLeftChannel).margin(0.001) );
    REQUIRE_THAT( rightChannel, Catch::Approx(expectedRightChannel).margin(0.001) );
}

TEST_CASE("[Helpers] Pan tests")
{
    // Testing different sizes to check that SIMD and unrolling works as expected
    panTest<1>(1.0f, 1.0f, 0.0f, 0.70711f, 0.70711f);
    panTest<1>(1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    panTest<1>(1.0f, 1.0f, -1.0f, 1.0f, 0.0f);
    panTest<3>(1.0f, 1.0f, 0.0f, 0.70711f, 0.70711f);
    panTest<3>(1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    panTest<3>(1.0f, 1.0f, -1.0f, 1.0f, 0.0f);
    panTest<10>(1.0f, 1.0f, 0.0f, 0.70711f, 0.70711f);
    panTest<10>(1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    panTest<10>(1.0f, 1.0f, -1.0f, 1.0f, 0.0f);
}

TEST_CASE("[Helpers] Width tests")
{
    widthTest<1>(1.0f, 1.0f, 0.0f, 1.414f, 1.414f);
    widthTest<1>(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    widthTest<1>(1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
    widthTest<3>(1.0f, 1.0f, 0.0f, 1.414f, 1.414f);
    widthTest<3>(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    widthTest<3>(1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
    widthTest<10>(1.0f, 1.0f, 0.0f, 1.414f, 1.414f);
    widthTest<10>(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    widthTest<10>(1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
}

TEST_CASE("[Helpers] clampAll")
{
    std::array<float, 10> inputScalar { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    std::array<float, 10> inputSIMD;
    sfz::copy<float>(inputScalar, absl::MakeSpan(inputSIMD));
    std::array<float, 10> expected { 2.5f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 8.0f, 8.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, false);
    sfz::clampAll<float>(absl::MakeSpan(inputScalar), 2.5f, 8.0f);
    REQUIRE(  approxEqual<float>(inputScalar, expected) );
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, true);
    sfz::clampAll<float>(absl::MakeSpan(inputSIMD), 2.5f, 8.0f);
    REQUIRE(  approxEqual<float>(inputSIMD, expected) );
}

TEST_CASE("[Helpers] clampAll (SIMD vs scalar)")
{
    std::vector<float> inputScalar(medBufferSize);
    std::vector<float> inputSIMD(medBufferSize);
    absl::c_iota(inputScalar, 2.0f);
    sfz::copy<float>(inputScalar, absl::MakeSpan(inputSIMD));
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, false);
    sfz::clampAll<float>(absl::MakeSpan(inputScalar), 10.0f, 50.0f);
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, true);
    sfz::clampAll<float>(absl::MakeSpan(inputSIMD), 10.0f, 50.0f);
    REQUIRE( approxEqual<float>(inputScalar, inputSIMD) );
}

TEST_CASE("[Helpers] allWithin")
{
    std::array<float, 10> input { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::allWithin, false);
    REQUIRE( sfz::allWithin<float>(input, 0.5f, 11.0f) );
    REQUIRE( !sfz::allWithin<float>(input, 2.5f, 8.0f) );
    REQUIRE( !sfz::allWithin<float>(input, 0.0f, 5.0f) );
    REQUIRE( !sfz::allWithin<float>(input, -1.0f, 7.0f) );
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::allWithin, true);
    REQUIRE( sfz::allWithin<float>(input, 0.5f, 11.0f) );
    REQUIRE( !sfz::allWithin<float>(input, 2.5f, 8.0f) );
    REQUIRE( !sfz::allWithin<float>(input, 0.0f, 5.0f) );
    REQUIRE( !sfz::allWithin<float>(input, -1.0f, 7.0f) );
}
