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

// NOTE: The goal is to collapse all checks on release, and compute the sentinels for the span versions
// https://godbolt.org/z/43dveW shows this should work

enum class SIMDOps {
    writeInterleaved,
    readInterleaved,
    fill,
    gain,
    divide,
    mathfuns,
    loopingSFZIndex,
    saturatingSFZIndex,
    linearRamp,
    multiplicativeRamp,
    add,
    subtract,
    multiplyAdd,
    copy,
    pan,
    cumsum,
    diff,
    sfzInterpolationCast,
    mean,
    meanSquared,
    upsampling,
    _sentinel //
};

// Enable or disable SIMD accelerators at runtime
void setSIMDOpStatus(SIMDOps op, bool status);
bool getSIMDOpStatus(SIMDOps op);

constexpr uintptr_t ByteAlignmentMask(unsigned N) { return N - 1; }

template<unsigned N = config::defaultAlignment, class T>
T* nextAligned(const T* ptr)
{
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) + ByteAlignmentMask(N) & (~ByteAlignmentMask(N)));
}

template<unsigned N = config::defaultAlignment, class T>
T* prevAligned(const T* ptr)
{
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) & (~ByteAlignmentMask(N)));
}

template<unsigned N = config::defaultAlignment, class T>
bool unaligned(const T* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & ByteAlignmentMask(N) )!= 0;
}

template<unsigned N = config::defaultAlignment, class T, class... Args>
bool unaligned(const T* ptr1, Args... rest)
{
    return unaligned<N>(ptr1) || unaligned<N>(rest...);
}

/**
 * @brief Read interleaved stereo data from a buffer and separate it in a left/right pair of buffers.
 *
 * @param input
 * @param outputLeft
 * @param outputRight
 * @param inputSize
 */
void readInterleaved(const float* input, float* outputLeft, float* outputRight, unsigned inputSize) noexcept;

inline void readInterleaved(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    // Something is fishy with the sizes
    CHECK(outputLeft.size() == input.size() / 2);
    CHECK(outputRight.size() == input.size() / 2);
    const auto size = min(input.size(), 2 * outputLeft.size(), 2 * outputRight.size());
    readInterleaved(input.data(), outputLeft.data(), outputRight.data(), size);
}

/**
 * @brief Write a pair of left and right stereo input into a single buffer interleaved.
 *
 * @param inputLeft
 * @param inputRight
 * @param output
 * @param outputSize
 */
void writeInterleaved(const float* inputLeft, const float* inputRight, float* output, unsigned outputSize) noexcept;

inline void writeInterleaved(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    // Something is fishy with the sizes
    CHECK(inputLeft.size() == output.size() / 2);
    CHECK(inputRight.size() == output.size() / 2);
    const auto size = min(output.size(), 2 * inputLeft.size(), 2 * inputRight.size());
    writeInterleaved(inputLeft.data(), inputRight.data(), output.data(), size);
}

/**
 * @brief Fill a buffer with a value
 *
 * @tparam T the underlying type
 * @param output
 * @param value
 */
template <class T>
void fill(absl::Span<T> output, T value) noexcept
{
    absl::c_fill(output, value);
}

/**
 * @brief Applies a scalar gain to the input
 *
 * @param gain the gain to apply
 * @param input
 * @param output
 * @param size
 */
template<class T>
void applyGain(T gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = gain * (*input++);
}

template<>
void applyGain<float>(float gain, const float* input, float* output, unsigned size) noexcept;

template<class T>
inline void applyGain(T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, output);
    applyGain<T>(gain, input.data(), output.data(), minSpanSize(input, output));
}

/**
 * @brief Applies a scalar gain inplace
 *
 * @param gain the gain to apply
 * @param array
 * @param size
 */
template<class T>
inline void applyGain(float gain, float* array, unsigned size) noexcept
{
    applyGain<T>(gain, array, array, size);
}

template<class T>
inline void applyGain(float gain, absl::Span<float> array) noexcept
{
    applyGain<T>(gain, array.data(), array.data(), array.size());
}

/**
 * @brief Applies a vector gain to an input span
 *
 * @param gain
 * @param input
 * @param output
 * @param size
 */
template<class T>
void applyGain(const T* gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = (*gain++) * (*input++);
}

template<>
void applyGain<float>(const float* gain, const float* input, float* output, unsigned size) noexcept;

template<class T>
inline void applyGain(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(gain, input, output);
    applyGain<T>(gain.data(), input.data(), output.data(), minSpanSize(gain, input, output));
}

/**
 * @brief Applies a vector gain in-place on a span
 *
 * @param gain
 * @param array
 * @param size
 */
template<class T>
inline void applyGain(const T* gain, T* array, unsigned size) noexcept
{
    applyGain<T>(gain, array, array, size);
}

template<class T>
inline void applyGain(absl::Span<const T> gain, absl::Span<T> array) noexcept
{
    CHECK_SPAN_SIZES(gain, array);
    applyGain<T>(gain.data(), array.data(), array.data(), minSpanSize(gain, array));
}

/**
 * @brief Divide a vector by another vector
 *
 * The output size will be the minimum of the divisor, input span and output span size.
 *
 * @tparam T the underlying type
 * @param input
 * @param divisor
 * @param output
 * @param size
 */
template <class T>
void divide(const T* input, const T* divisor, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = (*input++) / (*divisor++);
}

template <>
void divide<float>(const float* input, const float* divisor, float* output, unsigned size) noexcept;

template <class T>
inline void divide(absl::Span<const T> input, absl::Span<const T> divisor, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, divisor, output);
    divide<T>(input.data(), divisor.data(), output.data(), minSpanSize(input, divisor, output));
}

/**
 * @brief Divide a vector by another in place
 *
 * @tparam T the underlying type
 * @param output
 * @param divisor
 */
template <class T>
void divide(absl::Span<T> output,  absl::Span<const T> divisor) noexcept
{
    CHECK_SPAN_SIZES(divisor, output);
    divide<T>(output.data(), divisor.data(), output.data(), minSpanSize(divisor, output));
}

/**
 * @brief Applies a gain to the input and add it on the output
 *
 * @tparam T the underlying type
 * @param gain
 * @param input
 * @param output
 * @param size
 */
template <class T>
void multiplyAdd(const T* gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += (*gain++) * (*input++);
}

template <>
void multiplyAdd<float>(const float* gain, const float* input, float* output, unsigned size) noexcept;

template <class T>
void multiplyAdd(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(gain, input, output);
    multiplyAdd<T>(gain.data(), input.data(), output.data(), minSpanSize(gain, input, output));
}

/**
 * @brief Applies a gain to the input and add it on the output
 *
 * @tparam T the underlying type
 * @param gain
 * @param input
 * @param output
 * @param size
 */
template <class T>
void multiplyAdd(T gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += gain * (*input++);
}

template <>
void multiplyAdd<float>(float gain, const float* input, float* output, unsigned size) noexcept;

template <class T>
void multiplyAdd(T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, output);
    multiplyAdd<T>(gain, input.data(), output.data(), minSpanSize(input, output));
}

/**
 * @brief Compute a linear ramp blockwise between 2 values
 *
 * @tparam T the underlying type
 * @param output The destination span
 * @param start
 * @param step
 * @param size
 * @return T
 */
template <class T>
T linearRamp(T* output, T start, T step, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel) {
        *output++ = start;
        start += step;
    }
    return start;
}

template <>
float linearRamp<float>(float* output, float start, float step, unsigned size) noexcept;

template <class T>
T linearRamp(absl::Span<T> output, T start, T step) noexcept
{
    return linearRamp(output.data(), start, step, output.size());
}

/**
 * @brief Compute a multiplicative ramp blockwise between 2 values
 *
 * @tparam T the underlying type
 * @param output The destination span
 * @param start
 * @param step
 * @return T
 */
template <class T>
T multiplicativeRamp(T* output, T start, T step, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel) {
        *output++ = start;
        start *= step;
    }
    return start;
}

template <>
float multiplicativeRamp<float>(float* output, float start, float step, unsigned size) noexcept;

template <class T>
T multiplicativeRamp(absl::Span<T> output, T start, T step) noexcept
{
    return multiplicativeRamp(output.data(), start, step, output.size());

}

/**
 * @brief Add an input span to the output span
 *
 * @tparam T the underlying type
 * @param input
 * @param output
 * @param size
 */
template <class T>
void add(const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += *input++;
}

template <>
void add<float>(const float* input, float* output, unsigned size) noexcept;

template <class T>
void add(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, output);
    add<T>(input.data(), output.data(), minSpanSize(input, output));
}

/**
 * @brief Add a value inplace
 *
 * @tparam T the underlying type
 * @param value
 * @param output
 * @param size
 */
template <class T>
void add(T value, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += value;
}

template <>
void add<float>(float value, float* output, unsigned size) noexcept;

template <class T>
void add(T value, absl::Span<T> output) noexcept
{
    add<T>(value, output.data(), output.size());
}

/**
 * @brief Subtract an input span from the output span
 *
 * @tparam T the underlying type
 * @param input
 * @param output
 * @param size
 */
template <class T>
void subtract(const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ -= *input++;
}

template <>
void subtract<float>(const float* input, float* output, unsigned size) noexcept;

template <class T>
void subtract(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, output);
    subtract<T>(input.data(), output.data(), minSpanSize(input, output));
}

/**
 * @brief Subtract a value inplace
 *
 * @tparam T the underlying type
 * @param value
 * @param output
 * @param size
 */
template <class T>
void subtract(T value, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ -= value;
}

template <>
void subtract<float>(float value, float* output, unsigned size) noexcept;

template <class T>
void subtract(T value, absl::Span<T> output) noexcept
{
    subtract<T>(value, output.data(), output.size());
}

/**
 * @brief Copy a span in another
 *
 * @tparam T the underlying type
 * @param input
 * @param output
 * @param size
 */
template <class T>
void copy(const T* input, T* output, unsigned size) noexcept
{
    std::copy(input, input + size, output);
}

template <>
void copy<float>(const float* input, float* output, unsigned size) noexcept;

template <class T>
void copy(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    CHECK_SPAN_SIZES(input, output);
    copy<T>(input.data(), output.data(), minSpanSize(input, output));
}

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
    void snippetSFZInterpolationCast(const T*& floatJump, int*& jump, T*& coeff)
    {
        *jump = static_cast<int>(*floatJump);
        *coeff = *floatJump - static_cast<float>(*jump);
        incrementAll(floatJump, coeff, jump);
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
void sfzInterpolationCast(absl::Span<const T> floatJumps, absl::Span<int> jumps, absl::Span<T> coeffs) noexcept
{
    CHECK(jumps.size() >= floatJumps.size());
    CHECK(jumps.size() == coeffs.size());

    auto floatJump = floatJumps.data();
    auto jump = jumps.data();
    auto coeff = coeffs.data();
    const auto sentinel = floatJump + min(floatJumps.size(), jumps.size(), coeffs.size());

    while (floatJump < sentinel)
        _internals::snippetSFZInterpolationCast(floatJump, jump, coeff);
}

template <>
void sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> coeffs) noexcept;

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
