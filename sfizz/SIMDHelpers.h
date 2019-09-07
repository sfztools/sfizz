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

#pragma once
#include "Config.h"
#include "Debug.h"
#include "MathHelpers.h"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <cmath>

template <class T>
inline void snippetRead(const T*& input, T*& outputLeft, T*& outputRight)
{
    *outputLeft++ = *input++;
    *outputRight++ = *input++;
}

template <class T, bool SIMD = SIMDConfig::readInterleaved>
void readInterleaved(absl::Span<const T> input, absl::Span<T> outputLeft, absl::Span<T> outputRight) noexcept
{
    // The size of the output is not big enough for the input...
    ASSERT(outputLeft.size() >= input.size() / 2);
    ASSERT(outputRight.size() >= input.size() / 2);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();
    while (in < (input.end() - 1) && lOut < outputLeft.end() && rOut < outputRight.end())
        snippetRead<T>(in, lOut, rOut);
}

template <class T>
inline void snippetWrite(T*& output, const T*& inputLeft, const T*& inputRight)
{
    *output++ = *inputLeft++;
    *output++ = *inputRight++;
}

template <class T, bool SIMD = SIMDConfig::writeInterleaved>
void writeInterleaved(absl::Span<const T> inputLeft, absl::Span<const T> inputRight, absl::Span<T> output) noexcept
{
    ASSERT(inputLeft.size() <= output.size() / 2);
    ASSERT(inputRight.size() <= output.size() / 2);

    auto* lIn = inputLeft.begin();
    auto* rIn = inputRight.begin();
    auto* out = output.begin();
    while (lIn < inputLeft.end() && rIn < inputRight.end() && out < (output.end() - 1))
        snippetWrite<T>(out, lIn, rIn);
}

// Specializations
template <>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept;
template <>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept;

template <class T, bool SIMD = SIMDConfig::fill>
void fill(absl::Span<T> output, T value) noexcept
{
    absl::c_fill(output, value);
}

template <>
void fill<float, true>(absl::Span<float> output, float value) noexcept;

template <class Type, bool SIMD = SIMDConfig::mathfuns>
void exp(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::exp(input[i]);
}

template <>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class Type, bool SIMD = SIMDConfig::mathfuns>
void log(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::log(input[i]);
}

template <>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class Type, bool SIMD = SIMDConfig::mathfuns>
void sin(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::sin(input[i]);
}

template <>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class Type, bool SIMD = SIMDConfig::mathfuns>
void cos(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::cos(input[i]);
}

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

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
    index++;
    leftCoeff++;
    rightCoeff++;
    jump++;
}

template <class T, bool SIMD = SIMDConfig::saturatingSFZIndex>
float saturatingSFZIndex(absl::Span<const T> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs, absl::Span<int> indices, T floatIndex, T loopEnd) noexcept
{
    ASSERT(indices.size() >= jumps.size());
    ASSERT(indices.size() == leftCoeffs.size());
    ASSERT(indices.size() == rightCoeffs.size());

    auto* index = indices.begin();
    auto* leftCoeff = leftCoeffs.begin();
    auto* rightCoeff = rightCoeffs.begin();
    auto* jump = jumps.begin();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    auto* sentinel = jumps.begin() + size;

    while (jump < sentinel)
        snippetSaturatingIndex<T>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd);
    return floatIndex;
}

template <>
float saturatingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeffs, absl::Span<float> rightCoeffs, absl::Span<int> indices, float floatIndex, float loopEnd) noexcept;

template <class T>
inline void snippetLoopingIndex(const T*& jump, T*& leftCoeff, T*& rightCoeff, int*& index, T& floatIndex, T loopEnd, T loopStart)
{
    floatIndex += *jump;
    if (floatIndex >= loopEnd)
        floatIndex -= loopEnd - loopStart;
    *index = static_cast<int>(floatIndex);
    *rightCoeff = floatIndex - *index;
    *leftCoeff = 1.0f - *rightCoeff;
    index++;
    leftCoeff++;
    rightCoeff++;
    jump++;
}

template <class T, bool SIMD = SIMDConfig::loopingSFZIndex>
float loopingSFZIndex(absl::Span<const T> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs, absl::Span<int> indices, T floatIndex, T loopEnd, T loopStart) noexcept
{
    ASSERT(indices.size() >= jumps.size());
    ASSERT(indices.size() == leftCoeffs.size());
    ASSERT(indices.size() == rightCoeffs.size());

    auto* index = indices.begin();
    auto* leftCoeff = leftCoeffs.begin();
    auto* rightCoeff = rightCoeffs.begin();
    auto* jump = jumps.begin();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    auto* sentinel = jumps.begin() + size;

    while (jump < sentinel)
        snippetLoopingIndex<T>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd, loopStart);
    return floatIndex;
}

template <>
float loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept;

template <class T>
inline void snippetGain(T gain, const T*& input, T*& output)
{
    *output++ = gain * (*input++);
}

template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(input.size() <= output.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(output.size(), input.size());
    while (out < sentinel)
        snippetGain<T>(gain, in, out);
}

template <class T>
inline void snippetGainSpan(const T*& gain, const T*& input, T*& output)
{
    *output++ = (*gain++) * (*input++);
}

template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(gain.size() == input.size());
    ASSERT(input.size() <= output.size());
    auto* in = input.begin();
    auto* g = gain.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(gain.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
        snippetGainSpan<T>(g, in, out);
}

template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(T gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

template <class T, bool SIMD = SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

template <>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template <>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T>
inline void snippetMultiplyAdd(const T*& gain, const T*& input, T*& output)
{
    *output++ += (*gain++) * (*input++);
}

template <class T, bool SIMD = SIMDConfig::multiplyAdd>
void multiplyAdd(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(gain.size() == input.size());
    ASSERT(input.size() <= output.size());
    auto* in = input.begin();
    auto* g = gain.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(gain.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
        snippetMultiplyAdd<T>(g, in, out);
}

template <>
void multiplyAdd<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T>
inline void snippetRampLinear(T*& output, T& value, T step)
{
    value += step;
    *output++ = value;
}

template <class T, bool SIMD = SIMDConfig::linearRamp>
T linearRamp(absl::Span<T> output, T start, T step) noexcept
{
    auto* out = output.begin();
    while (out < output.end())
        snippetRampLinear<T>(out, start, step);
    return start;
}

template <class T>
inline void snippetRampMultiplicative(T*& output, T& value, T step)
{
    value *= step;
    *output++ = value;
}

template <class T, bool SIMD = SIMDConfig::multiplicativeRamp>
T multiplicativeRamp(absl::Span<T> output, T start, T step) noexcept
{
    auto* out = output.begin();
    while (out < output.end())
        snippetRampMultiplicative<T>(out, start, step);
    return start;
}

template <>
float linearRamp<float, true>(absl::Span<float> output, float start, float step) noexcept;

template <>
float multiplicativeRamp<float, true>(absl::Span<float> output, float start, float step) noexcept;

template <class T>
inline void snippetAdd(const T*& input, T*& output)
{
    *output++ += *input++;
}

template <class T, bool SIMD = SIMDConfig::add>
void add(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        snippetAdd(in, out);
}

template <>
void add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T>
inline void snippetSubtract(const T*& input, T*& output)
{
    *output++ -= *input++;
}

template <class T, bool SIMD = SIMDConfig::subtract>
void subtract(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        snippetSubtract(in, out);
}

template <>
void subtract<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;


template <class T>
void snippetCopy(const T*& input, T*& output)
{
    *output++ = *input++;
}

template <class T, bool SIMD = SIMDConfig::copy>
void copy(absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    while (out < sentinel)
        snippetCopy(in, out);
}

template <>
void copy<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template <class T>
inline void snippetPan(const T*& pan, T*& left, T*& right)
{
    const auto circlePan = piFour<float> * (static_cast<T>(1.0) + *pan++);
    *left++ *= std::cos(circlePan);
    *right++ *= std::sin(circlePan);
}

template <class T, bool SIMD = SIMDConfig::pan>
void pan(absl::Span<const T> panEnvelope, absl::Span<T> leftBuffer, absl::Span<T> rightBuffer) noexcept
{
    ASSERT(leftBuffer.size() >= panEnvelope.size());
    ASSERT(rightBuffer.size() >= panEnvelope.size());
    auto* pan = panEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = pan + min(panEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    while (pan < sentinel)
        snippetPan(pan, left, right);
}

template <>
void pan<float, true>(absl::Span<const float> panEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept;