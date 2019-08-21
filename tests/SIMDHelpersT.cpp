#include "catch2/catch.hpp"
#include "../sources/SIMDHelpers.h"
#include <array>
#include <algorithm>
#include <absl/types/span.h>
#include <absl/algorithm/container.h>
using namespace Catch::literals;

constexpr int smallBufferSize { 3 };
constexpr int bigBufferSize { 4095 };
constexpr int medBufferSize { 127 };
constexpr double fillValue { 1.3 };

template<class Type>
inline bool approxEqual(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps=1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).epsilon(eps))
        {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

TEST_CASE("[Helpers] fill() - Manual buffer")
{
    std::vector<float> buffer (5);
    std::vector<float> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer")
{
    std::vector<float> buffer (smallBufferSize);
    std::vector<float> expected (smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer")
{
    std::vector<float> buffer (bigBufferSize);
    std::vector<float> expected (bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<float, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer -- SIMD")
{
    std::vector<float> buffer (smallBufferSize);
    std::vector<float> expected (smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<float, true>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer -- SIMD")
{
    std::vector<float> buffer (bigBufferSize);
    std::vector<float> expected (bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<float, true>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Small buffer -- doubles")
{
    std::vector<double> buffer (smallBufferSize);
    std::vector<double> expected (smallBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<double, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}

TEST_CASE("[Helpers] fill() - Big buffer -- doubles")
{
    std::vector<double> buffer (bigBufferSize);
    std::vector<double> expected (bigBufferSize);
    std::fill(expected.begin(), expected.end(), fillValue);

    fill<double, false>(absl::MakeSpan(buffer), fillValue);
    REQUIRE(buffer == expected);
}


TEST_CASE("[Helpers] Interleaved read")
{
    std::array<float, 16> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f};
    std::array<float, 16> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 8> leftOutput;
    std::array<float, 8> rightOutput;
    readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 16> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Interleaved read unaligned end")
{
    std::array<float, 20> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f};
    std::array<float, 20> expected { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f};
    std::array<float, 10> leftOutput;
    std::array<float, 10> rightOutput;
    readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 20> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Small interleaved read unaligned end")
{
    std::array<float, 6> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f};
    std::array<float, 6> expected { 0.0f, 1.0f, 2.0f, 10.0f, 11.0f, 12.0f};
    std::array<float, 3> leftOutput;
    std::array<float, 3> rightOutput;
    readInterleaved<float, false>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 6> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Interleaved read -- SIMD")
{
    std::array<float, 16> input = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f};
    std::array<float, 16> expected = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 8> leftOutput;
    std::array<float, 8> rightOutput;
    readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 16> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Interleaved read unaligned end -- SIMD")
{
    std::array<float, 20> input = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f};
    std::array<float, 20> expected = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f};
    std::array<float, 10> leftOutput;
    std::array<float, 10> rightOutput;
    readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 20> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Small interleaved read unaligned end -- SIMD")
{
    std::array<float, 6> input { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f};
    std::array<float, 6> expected { 0.0f, 1.0f, 2.0f, 10.0f, 11.0f, 12.0f};
    std::array<float, 3> leftOutput;
    std::array<float, 3> rightOutput;
    readInterleaved<float, true>(input, absl::MakeSpan(leftOutput), absl::MakeSpan(rightOutput));
    std::array<float, 6> real;

    auto realIdx = 0;
    for (auto value: leftOutput)
        real[realIdx++] = value;
    for (auto value: rightOutput)
        real[realIdx++] = value;
    REQUIRE( real == expected );
}

TEST_CASE("[Helpers] Interleaved read SIMD vs Scalar")
{
    std::array<float, medBufferSize * 2> input;
    std::array<float, medBufferSize> leftOutputScalar;
    std::array<float, medBufferSize> rightOutputScalar;
    std::array<float, medBufferSize> leftOutputSIMD;
    std::array<float, medBufferSize> rightOutputSIMD;
    std::iota(input.begin(), input.end(), 0.0f);
    readInterleaved<float, false>(input, absl::MakeSpan(leftOutputScalar), absl::MakeSpan(rightOutputScalar));
    readInterleaved<float, true>(input, absl::MakeSpan(leftOutputSIMD), absl::MakeSpan(rightOutputSIMD));
    REQUIRE( leftOutputScalar == leftOutputSIMD );
    REQUIRE( rightOutputScalar == rightOutputSIMD );
}

TEST_CASE("[Helpers] Interleaved write")
{
    std::array<float, 8> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, };
    std::array<float, 8> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 16> output;
    std::array<float, 16> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f};
    writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Interleaved write unaligned end")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f};
    writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Small interleaved write unaligned end")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f};
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f};
    writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Interleaved write -- SIMD")
{
    std::array<float, 8> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, };
    std::array<float, 8> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };
    std::array<float, 16> output;
    std::array<float, 16> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f};
    writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Interleaved write unaligned end -- SIMD")
{
    std::array<float, 10> leftInput { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    std::array<float, 10> rightInput { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };
    std::array<float, 20> output;
    std::array<float, 20> expected = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f};
    writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Small interleaved write unaligned end -- SIMD")
{
    std::array<float, 3> leftInput { 0.0f, 1.0f, 2.0f};
    std::array<float, 3> rightInput { 10.0f, 11.0f, 12.0f };
    std::array<float, 6> output;
    std::array<float, 6> expected { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f};
    writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Interleaved write SIMD vs Scalar")
{
    std::array<float, medBufferSize> leftInput;
    std::array<float, medBufferSize> rightInput;
    std::array<float, medBufferSize * 2> outputScalar;
    std::array<float, medBufferSize * 2> outputSIMD;
    std::iota(leftInput.begin(), leftInput.end(), 0.0f);
    std::iota(rightInput.begin(), rightInput.end(), medBufferSize);
    writeInterleaved<float, false>(leftInput, rightInput, absl::MakeSpan(outputScalar));
    writeInterleaved<float, true>(leftInput, rightInput, absl::MakeSpan(outputSIMD));
    REQUIRE( outputScalar == outputSIMD );
}

TEST_CASE("[Helpers] Gain, single")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    applyGain<float, false>(fillValue, input, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Gain, single and inplace")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    applyGain<float, false>(fillValue, buffer, absl::MakeSpan(buffer));
    REQUIRE( buffer == expected );
}

TEST_CASE("[Helpers] Gain, spans")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    applyGain<float, false>(gain, input, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Gain, spans and inplace")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    applyGain<float, false>(gain, buffer, absl::MakeSpan(buffer));
    REQUIRE( buffer == expected );
}

TEST_CASE("[Helpers] Gain, single (SIMD)")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    applyGain<float, true>(fillValue, input, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Gain, single and inplace (SIMD)")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> expected { fillValue, fillValue, fillValue, fillValue, fillValue };
    applyGain<float, true>(fillValue, buffer, absl::MakeSpan(buffer));
    REQUIRE( buffer == expected );
}

TEST_CASE("[Helpers] Gain, spans (SIMD)")
{
    std::array<float, 5> input { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> output { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    applyGain<float, true>(gain, input, absl::MakeSpan(output));
    REQUIRE( output == expected );
}

TEST_CASE("[Helpers] Gain, spans and inplace (SIMD)")
{
    std::array<float, 5> buffer { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 5> gain { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::array<float, 5> expected { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    applyGain<float, true>(gain, buffer, absl::MakeSpan(buffer));
    REQUIRE( buffer == expected );
}

TEST_CASE("[Helpers] SFZ looping index")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f}; // 1.1 2.3 3.6 5.0 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 1, 2, 4 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 1.0f, 0.5f, 0.9f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 0.0f, 0.5f, 0.1f };
    loopingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6, 1);
    REQUIRE( indices == expectedIndices );
    REQUIRE( approxEqual<float>(leftCoeffs, expectedLeft) );
    REQUIRE( approxEqual<float>(rightCoeffs, expectedRight) );
}

TEST_CASE("[Helpers] SFZ looping index (SIMD)")
{
    std::array<float, 6> jumps { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f}; // 1.1 2.3 3.6 5.0 6.5 8.1
    std::array<int, 6> indices;
    std::array<float, 6> leftCoeffs;
    std::array<float, 6> rightCoeffs;
    std::array<int, 6> expectedIndices { 2, 3, 4, 1, 2, 4 };
    std::array<float, 6> expectedLeft { 0.9f, 0.7f, 0.4f, 1.0f, 0.5f, 0.9f };
    std::array<float, 6> expectedRight { 0.1f, 0.3f, 0.6f, 0.0f, 0.5f, 0.1f };
    loopingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, 6, 1);
    REQUIRE( indices == expectedIndices );
    REQUIRE( approxEqual<float>(leftCoeffs, expectedLeft) );
    REQUIRE( approxEqual<float>(rightCoeffs, expectedRight) );
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
//     loopingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 1.0f, medBufferSize, 1);
//     loopingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffsSIMD), absl::MakeSpan(rightCoeffsSIMD), absl::MakeSpan(indicesSIMD), 1.0f, medBufferSize, 1);
//     REQUIRE( approxEqual<int>(indices, indicesSIMD, 1) );
//     REQUIRE( approxEqual<float>(leftCoeffs, leftCoeffsSIMD, 1e-2) );
//     REQUIRE( approxEqual<float>(rightCoeffs, rightCoeffsSIMD, 1e-2) );
// }