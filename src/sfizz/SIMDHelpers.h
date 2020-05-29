// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @file SIMDHelpers.h
 * @brief This file contains useful functions to treat buffers of numerical values
 * (e.g. a buffer of floats usually).
 *
 * These functions are templated to apply on
 * various underlying buffer types, and this file contains the generic version of the
 * function. Some templates specializations exists for different architecture that try
 * to make use of SIMD intrinsics; you can find such a file in SIMDSSE.cpp and possibly
 * someday SIMDNEON.cpp for ARM platforms.
 *
 * If you want to write specializations for float buffers the idea is to start from the SIMDDummy
 * file that just calls back the generic implementation, and implement the specializations you
 * wish from this list. You can then either activate or deactivate a SIMD version by default
 * using the variables in Config.h, or call e.g. writeInterleaved<float, true>(...) to use the
 * SIMD version of writeInterleaved. To implement e.g. double template specializations you
 * will need to amend this file to pre-declare the specializations, and create a file similar to
 * SIMDxxx.cpp.
 *
 * All the SIMD functions are benchmarked. If you run the benchmark for a given function you can check
 * if it is interesting to run the SIMD version by default. The interest is that you can activate
 * and deactivate each SIMD specialization with a fine granularity, since SIMD performance
 * will be very dependent on the processor architecture. Modern processors can also organize their
 * instructions so that scalar non-SIMD code runs sometimes much more efficiently than SIMD code
 * especially when the latter does not operate on misaligned buffers.
 *
 */
#pragma once
#include "Config.h"
#include "Debug.h"
#include "MathHelpers.h"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <array>
#include <cmath>

namespace sfz {
namespace _internals {
    template <class T>
    inline void snippetRead(const T*& input, T*& outputLeft, T*& outputRight)
    {
        *outputLeft++ = *input++;
        *outputRight++ = *input++;
    }
}

/**
 * @brief Read interleaved stereo data from a buffer and separate it in a left/right pair of buffers.
 *
 * The output size will be the minimum of the input span and output spans size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param outputLeft
 * @param outputRight
 */
template <class T, bool SIMD = SIMDConfig::readInterleaved>
void readInterleaved(absl::Span<const T> input, absl::Span<T> outputLeft, absl::Span<T> outputRight) noexcept
{
    // The size of the output is not big enough for the input...
    CHECK(outputLeft.size() >= input.size() / 2);
    CHECK(outputRight.size() >= input.size() / 2);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();
    while (in < (input.end() - 1) && lOut < outputLeft.end() && rOut < outputRight.end())
        _internals::snippetRead<T>(in, lOut, rOut);
}

namespace _internals {
    template <class T>
    inline void snippetWrite(T*& output, const T*& inputLeft, const T*& inputRight)
    {
        *output++ = *inputLeft++;
        *output++ = *inputRight++;
    }
}

/**
 * @brief Write a pair of left and right stereo input into a single buffer interleaved.
 *
 * The output size will be the minimum of the input spans and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param inputLeft
 * @param inputRight
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::writeInterleaved>
void writeInterleaved(absl::Span<const T> inputLeft, absl::Span<const T> inputRight, absl::Span<T> output) noexcept
{
    CHECK(inputLeft.size() <= output.size() / 2);
    CHECK(inputRight.size() <= output.size() / 2);

    auto* lIn = inputLeft.begin();
    auto* rIn = inputRight.begin();
    auto* out = output.begin();
    while (lIn < inputLeft.end() && rIn < inputRight.end() && out < (output.end() - 1))
        _internals::snippetWrite<T>(out, lIn, rIn);
}

// Specializations
template <>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept;
template <>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept;

/**
 * @brief Fill a buffer with a value; comparable to std::fill in essence.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param output
 * @param value
 */
template <class T, bool SIMD = SIMDConfig::fill>
void fill(absl::Span<T> output, T value) noexcept
{
    absl::c_fill(output, value);
}

template <>
void fill<float, true>(absl::Span<float> output, float value) noexcept;

/**
 * @brief Exp math function
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class Type, bool SIMD = SIMDConfig::mathfuns>
void exp(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::exp(input[i]);
}

template <>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

/**
 * @brief Log math function
 *
 * The output size will be the minimum of the input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class Type, bool SIMD = SIMDConfig::mathfuns>
void log(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::log(input[i]);
}

template <>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

/**
 * @brief sin math function
 *
 * The output size will be the minimum of the input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class Type, bool SIMD = SIMDConfig::mathfuns>
void sin(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::sin(input[i]);
}

template <>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

/**
 * @brief cos math function
 *
 * The output size will be the minimum of the input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class Type, bool SIMD = SIMDConfig::mathfuns>
void cos(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::cos(input[i]);
}

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    inline void snippetSaturatingIndex(const T*& jump, T*& leftCoeff, T*& rightCoeff, int*& index, T& floatIndex, T loopEnd)
    {
        floatIndex += *jump;
        if (floatIndex >= loopEnd) {
            floatIndex = loopEnd;
            *index = static_cast<int>(floatIndex) - 1;
            *rightCoeff = static_cast<T>(1.0);
            *leftCoeff = static_cast<T>(0.0);
        } else {
            *index = static_cast<int>(floatIndex);
            *rightCoeff = floatIndex - *index;
            *leftCoeff = static_cast<T>(1.0) - *rightCoeff;
        }
        incrementAll(index, leftCoeff, rightCoeff, jump);
    }
}

/**
 * @brief Computes an integer index and 2 float coefficients corresponding to the
 * linear interpolation procedure. This version will saturate the index to the upper
 * bound if the upper bound is reached.
 *
 * The indices are computed starting from the given floatIndex, and each increment
 * is given by the elements of jumps.
 * The output size will be the minimum of the inputs span and outputs span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param jumps the floating point increments to the index
 * @param leftCoeffs the linear interpolation coefficients for the left value
 * @param rightCoeffs the linear interpolation coefficients for the right value
 * @param indices the integer sample indices for the left values; the right values
 *          for interpolation at index i are (indices[i] + 1) and not indices[i+1]
 * @param floatIndex the starting floating point index
 * @param loopEnd the end of the "loop" which is not really a loop because it saturate.
 * @return float
 */
template <class T, bool SIMD = SIMDConfig::saturatingSFZIndex>
float saturatingSFZIndex(absl::Span<const T> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs, absl::Span<int> indices, T floatIndex, T loopEnd) noexcept
{
    CHECK(indices.size() >= jumps.size());
    CHECK(indices.size() == leftCoeffs.size());
    CHECK(indices.size() == rightCoeffs.size());

    auto* index = indices.begin();
    auto* leftCoeff = leftCoeffs.begin();
    auto* rightCoeff = rightCoeffs.begin();
    auto* jump = jumps.begin();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    auto* sentinel = jumps.begin() + size;

    while (jump < sentinel)
        _internals::snippetSaturatingIndex<T>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd);
    return floatIndex;
}

template <>
float saturatingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeffs, absl::Span<float> rightCoeffs, absl::Span<int> indices, float floatIndex, float loopEnd) noexcept;

namespace _internals {
    template <class T>
    inline void snippetLoopingIndex(const T*& jump, T*& leftCoeff, T*& rightCoeff, int*& index, T& floatIndex, T loopEnd, T loopStart)
    {
        floatIndex += *jump;
        if (floatIndex >= loopEnd)
            floatIndex -= loopEnd - loopStart;
        *index = static_cast<int>(floatIndex);
        *rightCoeff = floatIndex - *index;
        *leftCoeff = 1.0f - *rightCoeff;
        incrementAll(index, leftCoeff, rightCoeff, jump);
    }
}

/**
 * @brief Computes an integer index and 2 float coefficients corresponding to the
 * linear interpolation procedure. This version will loop the index at the upper
 * bound loopend and restart it at the start of the loop.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param jumps the floating point increments to the index
 * @param leftCoeffs the linear interpolation coefficients for the left value
 * @param rightCoeffs the linear interpolation coefficients for the right value
 * @param indices the integer sample indices for the left values; the right values
 *          for interpolation at index i are (indices[i] + 1) and not indices[i+1]
 * @param floatIndex the starting floating point index
 * @param loopEnd the end index of the loop
 * @param loopStart the start index of the loop
 * @return float
 */
template <class T, bool SIMD = SIMDConfig::loopingSFZIndex>
float loopingSFZIndex(absl::Span<const T> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs, absl::Span<int> indices, T floatIndex, T loopEnd, T loopStart) noexcept
{
    CHECK(indices.size() >= jumps.size());
    CHECK(indices.size() == leftCoeffs.size());
    CHECK(indices.size() == rightCoeffs.size());

    auto* index = indices.begin();
    auto* leftCoeff = leftCoeffs.begin();
    auto* rightCoeff = rightCoeffs.begin();
    auto* jump = jumps.begin();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    auto* sentinel = jumps.begin() + size;

    while (jump < sentinel)
        _internals::snippetLoopingIndex<T>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd, loopStart);
    return floatIndex;
}

template <>
float loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept;

namespace _internals {
    template <class T>
    inline void snippetGain(T gain, const T*& input, T*& output)
    {
        *output++ = gain * (*input++);
    }
}

/**
 * @brief Applies a scalar gain to the input
 *
 * The output size will be the minimum of the input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param gain the gain to apply
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(input.size() <= output.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(output.size(), input.size());
    while (out < sentinel)
        _internals::snippetGain<T>(gain, in, out);
}

namespace _internals {
    template <class T>
    inline void snippetGainSpan(const T*& gain, const T*& input, T*& output)
    {
        *output++ = (*gain++) * (*input++);
    }
}

/**
 * @brief Applies a vector gain to an input stap
 *
 * The output size will be the minimum of the gain, input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param gain
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(gain.size() == input.size());
    CHECK(input.size() <= output.size());
    auto* in = input.begin();
    auto* g = gain.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(gain.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
        _internals::snippetGainSpan<T>(g, in, out);
}

/**
 * @brief Applies a scalar gain in-place on a span
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param gain
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(T gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

/**
 * @brief Applies a vector gain in-place on a span
 *
 * The output size will be the minimum of the gain span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param gain
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

template <>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template <>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    inline void snippetDivSpan(const T*& input, const T*& divisor,T*& output)
    {
        *output++ = (*input++) / (*divisor++);
    }
}

/**
 * @brief Divide a vector by another vector
 *
 * The output size will be the minimum of the divisor, input span and output span size.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param divisor
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::divide>
void divide(absl::Span<const T> input, absl::Span<const T> divisor, absl::Span<T> output) noexcept
{
    CHECK(divisor.size() == input.size());
    CHECK(input.size() <= output.size());
    auto* in = input.begin();
    auto* d = divisor.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(divisor.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
        _internals::snippetDivSpan<T>(in, d, out);
}

/**
 * @brief Divide a vector by another in place
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param output
 * @param divisor
 */
template <class T, bool SIMD = SIMDConfig::divide>
void divide(absl::Span<T> output,  absl::Span<const T> divisor) noexcept
{
    divide<T, SIMD>(output, divisor, output);
}

template <>
void divide<float, true>(absl::Span<const float> input, absl::Span<const float> divisor, absl::Span<float> output) noexcept;


namespace _internals {
    template <class T>
    inline void snippetMultiplyAdd(const T*& gain, const T*& input, T*& output)
    {
        *output++ += (*gain++) * (*input++);
    }

    template <class T>
    inline void snippetMultiplyAdd(const T gain, const T*& input, T*& output)
    {
        *output++ += gain * (*input++);
    }
}

/**
 * @brief Applies a gain to the input and add it on the output
 *
 * The output size will be the minimum of the gain span, input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param gain
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::multiplyAdd>
void multiplyAdd(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(gain.size() == input.size());
    CHECK(input.size() <= output.size());
    auto* in = input.begin();
    auto* g = gain.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(gain.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
        _internals::snippetMultiplyAdd<T>(g, in, out);
}

template <>
void multiplyAdd<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T, bool SIMD = SIMDConfig::multiplyAdd>
void multiplyAdd(const T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    // CHECK(input.size() <= output.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(output.size(), input.size());
    while (out < sentinel)
        _internals::snippetMultiplyAdd<T>(gain, in, out);
}

template <>
void multiplyAdd<float, true>(const float gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    inline void snippetRampLinear(T*& output, T& value, T step)
    {
        *output++ = value;
        value += step;
    }
}

/**
 * @brief Compute a linear ramp blockwise between 2 values
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param output The destination span
 * @param start
 * @param step
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::linearRamp>
T linearRamp(absl::Span<T> output, T start, T step) noexcept
{
    auto* out = output.begin();
    while (out < output.end())
        _internals::snippetRampLinear<T>(out, start, step);
    return start;
}

namespace _internals {
    template <class T>
    inline void snippetRampMultiplicative(T*& output, T& value, T step)
    {
        *output++ = value;
        value *= step;
    }
}

/**
 * @brief Compute a multiplicative ramp blockwise between 2 values
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param output The destination span
 * @param start
 * @param step
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::multiplicativeRamp>
T multiplicativeRamp(absl::Span<T> output, T start, T step) noexcept
{
    auto* out = output.begin();
    while (out < output.end())
        _internals::snippetRampMultiplicative<T>(out, start, step);
    return start;
}

template <>
float linearRamp<float, true>(absl::Span<float> output, float start, float step) noexcept;

template <>
float multiplicativeRamp<float, true>(absl::Span<float> output, float start, float step) noexcept;

namespace _internals {
    template <class T>
    inline void snippetAdd(const T*& input, T*& output)
    {
        *output++ += *input++;
    }
    template <class T>
    inline void snippetAdd(const T value, T*& output)
    {
        *output++ += value;
    }
}

/**
 * @brief Add an input span to the output span
 *
 * The output size will be the minimum of the gain span, input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::add>
void add(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        _internals::snippetAdd(in, out);
}

template <>
void add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T, bool SIMD = SIMDConfig::add>
void add(T value, absl::Span<T> output) noexcept
{
    auto* out = output.begin();
    auto* sentinel = output.end();
    while (out < sentinel)
        _internals::snippetAdd(value, out);
}

template <>
void add<float, true>(float value, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    inline void snippetSubtract(const T*& input, T*& output)
    {
        *output++ -= *input++;
    }

    template <class T>
    inline void snippetSubtract(const T value, T*& output)
    {
        *output++ -= value;
    }
}

/**
 * @brief Subtract a value from a span
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param value
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::subtract>
void subtract(const T value, absl::Span<T> output) noexcept
{
    auto* out = output.begin();
    auto* sentinel = output.end();
    while (out < sentinel)
        _internals::snippetSubtract(value, out);
}

/**
 * @brief Subtract a span from another span
 *
 * The output size will be the minimum of the input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::subtract>
void subtract(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        _internals::snippetSubtract(in, out);
}

template <>
void subtract<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <>
void subtract<float, true>(const float value, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    void snippetCopy(const T*& input, T*& output)
    {
        *output++ = *input++;
    }
}

/**
 * @brief Copy a span in another
 *
 * The output size will be the minimum of the input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param input
 * @param output
 */
template <class T, bool SIMD = SIMDConfig::copy>
void copy(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (output.data() == input.data() && output.size() == input.size())
        return;
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        _internals::snippetCopy(in, out);
}

template <>
void copy<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

namespace _internals {
    // Number of elements in the table, odd for equal volume at center
    constexpr int panSize = 4095;

    // Table of pan values for the left channel, extra element for safety
    const auto panData = []()
    {
        std::array<float, panSize + 1> pan;
        int i = 0;

        for (; i < panSize; ++i)
            pan[i] = std::cos(i * (piTwo<double>() / (panSize - 1)));

        for (; i < static_cast<int>(pan.size()); ++i)
            pan[i] = pan[panSize - 1];

        return pan;
    }();

    template <class T>
    inline T panLookup(T pan)
    {
        // reduce range, round to nearest
        int index = static_cast<int>(T{0.5} + pan * (panSize - 1));
        return panData[index];
    }

    template <class T>
    inline void snippetPan(T pan, T& left, T& right)
    {
        pan = (pan + T{1.0}) * T{0.5};
        pan = clamp<T>(pan, 0, 1);
        left *= panLookup(pan);
        right *= panLookup(1 - pan);
    }

    template <class T>
    inline void snippetWidth(T width, T& left, T& right)
    {
        T w = (width + T{1.0}) * T{0.5};
        w = clamp<T>(w, 0, 1);
        const auto coeff1 = panLookup(w);
        const auto coeff2 = panLookup(1 - w);
        const auto l = left;
        const auto r = right;
        left = l * coeff2 + r * coeff1;
        right = l * coeff1 + r * coeff2;
    }
}

/**
 * @brief Pans a mono signal left or right
 *
 * The output size will be the minimum of the pan envelope span and left and right buffer span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param panEnvelope
 * @param leftBuffer
 * @param rightBuffer
 */
template <class T, bool SIMD = SIMDConfig::pan>
void pan(absl::Span<const T> panEnvelope, absl::Span<T> leftBuffer, absl::Span<T> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= panEnvelope.size());
    CHECK(rightBuffer.size() >= panEnvelope.size());
    auto* pan = panEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = pan + min(panEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    while (pan < sentinel) {
        _internals::snippetPan(*pan, *left, *right);
        incrementAll(pan, left, right);
    }
}

template <>
void pan<float, true>(absl::Span<const float> panEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept;

/**
 * @brief Controls the width of a stereo signal, setting it to mono when width = 0 and inverting the channels
 * when width = -1. Width = 1 has no effect.
 *
 * The output size will be the minimum of the width envelope span and left and right buffer span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param panEnvelope
 * @param leftBuffer
 * @param rightBuffer
 */
template <class T, bool SIMD = SIMDConfig::pan>
void width(absl::Span<const T> widthEnvelope, absl::Span<T> leftBuffer, absl::Span<T> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= widthEnvelope.size());
    CHECK(rightBuffer.size() >= widthEnvelope.size());
    auto* width = widthEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = width + min(widthEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    while (width < sentinel) {
        _internals::snippetWidth(*width, *left, *right);
        incrementAll(width, left, right);
    }
}

template <>
void width<float, true>(absl::Span<const float> widthEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept;

/**
 * @brief Computes the mean of a span
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param vector
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::mean>
T mean(absl::Span<const T> vector) noexcept
{
    T result{ 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    while (value < vector.end())
        result += *value++;

    return result / static_cast<T>(vector.size());
}

template <>
float mean<float, true>(absl::Span<const float> vector) noexcept;

/**
 * @brief Computes the mean squared of a span
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param vector
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::meanSquared>
T meanSquared(absl::Span<const T> vector) noexcept
{
    T result{ 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    while (value < vector.end()) {
        result += (*value) * (*value);
        value++;
    }

    return result / static_cast<T>(vector.size());
}

template <>
float meanSquared<float, true>(absl::Span<const float> vector) noexcept;

namespace _internals {
    template <class T>
    inline void snippetCumsum(const T*& input, T*& output)
    {
        *output = *(output - 1) + *input++;
        output++;
    }
}

/**
 * @brief Computes the cumulative sum of a span.
 * The first output is the same as the first input.
 *
 * The output size will be the minimum of the input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param vector
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::cumsum>
void cumsum(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());

    *out++ = *in++;
    while (in < sentinel)
        _internals::snippetCumsum(in, out);
}

template <>
void cumsum<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

namespace _internals {
    template <class T>
    void snippetSFZInterpolationCast(const T*& floatJump, int*& jump, T*& leftCoeff, T*& rightCoeff)
    {
        *jump = static_cast<int>(*floatJump);
        *rightCoeff = *floatJump - static_cast<float>(*jump);
        *leftCoeff = static_cast<T>(1.0) - *rightCoeff;
        incrementAll(floatJump, leftCoeff, rightCoeff, jump);
    }
}

/**
 * @brief Computes the linear interpolation coefficients for a floating point index
 * and extracts the integer index of the elements to interpolate
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param floatJumps the floating point indices
 * @param jumps the integer indices outputs
 * @param leftCoeffs the left interpolation coefficients
 * @param rightCoeffs the right interpolation coefficients
 */
template <class T, bool SIMD = SIMDConfig::sfzInterpolationCast>
void sfzInterpolationCast(absl::Span<const T> floatJumps, absl::Span<int> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs) noexcept
{
    CHECK(jumps.size() >= floatJumps.size());
    CHECK(jumps.size() == leftCoeffs.size());
    CHECK(jumps.size() == rightCoeffs.size());

    auto floatJump = floatJumps.data();
    auto jump = jumps.data();
    auto leftCoeff = leftCoeffs.data();
    auto rightCoeff = rightCoeffs.data();
    const auto sentinel = floatJump + min(floatJumps.size(), jumps.size(), leftCoeffs.size(), rightCoeffs.size());

    while (floatJump < sentinel)
        _internals::snippetSFZInterpolationCast(floatJump, jump, leftCoeff, rightCoeff);
}

template <>
void sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> leftCoeffs, absl::Span<float> rightCoeffs) noexcept;

namespace _internals {
    template <class T>
    inline void snippetDiff(const T*& input, T*& output)
    {
        *output = *input - *(input - 1);
        output++;
        input++;
    }
}

/**
 * @brief Computes the differential of a span (successive differences).
 * The first output is the same as the first input.
 *
 * The output size will be the minimum of the input span and output span sizes.
 *
 * @tparam T the underlying type
 * @tparam SIMD use the SIMD version or the scalar version
 * @param vector
 * @return T
 */
template <class T, bool SIMD = SIMDConfig::diff>
void diff(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());

    *out++ = *in++;
    while (in < sentinel)
        _internals::snippetDiff(in, out);
}

template <>
void diff<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

} // namespace sfz
