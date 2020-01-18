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

#include <arm_neon.h>
#include "SIMDHelpers.h"

using Type = float;
[[maybe_unused]] constexpr uintptr_t TypeAlignment { 4 };
[[maybe_unused]] constexpr uintptr_t TypeAlignmentMask { TypeAlignment - 1 };
[[maybe_unused]] constexpr uintptr_t ByteAlignment { TypeAlignment * sizeof(Type) };
[[maybe_unused]] constexpr uintptr_t ByteAlignmentMask { ByteAlignment - 1 };

float* nextAligned(const float* ptr)
{
    return reinterpret_cast<float*>((reinterpret_cast<uintptr_t>(ptr) + ByteAlignmentMask) & (~ByteAlignmentMask));
}

float* prevAligned(const float* ptr)
{
    return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(ptr) & (~ByteAlignmentMask));
}

bool unaligned(const float* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & ByteAlignmentMask) != 0;
}

template<class... Args>
bool unaligned(const float* ptr1, Args... rest)
{
    return unaligned(ptr1) || unaligned(rest...);
}

template <>
void sfz::readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    // The size of the outputs is not big enough for the input...
    ASSERT(outputLeft.size() >= input.size() / 2);
    ASSERT(outputRight.size() >= input.size() / 2);
    // Input is too small
    ASSERT(input.size() > 1);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();

    const auto size = std::min(input.size(), std::min(outputLeft.size() * 2, outputRight.size() * 2));
    const auto* lastAligned = prevAligned(input.begin() + size - TypeAlignment);

    while (unaligned(in, lOut, rOut) && in < lastAligned)
        _internals::snippetRead<float>(in, lOut, rOut);

    while (in < lastAligned) {
        auto reg = vld2q_f32(in);
        vst1q_f32(lOut, reg.val[0]);
        vst1q_f32(rOut, reg.val[1]);
        // *lOut = reg.val[0];
        // *rOut = reg.val[1];
        incrementAll<TypeAlignment>(in, in, lOut, rOut);
    }

    while (in < input.end() - 1)
        _internals::snippetRead<float>(in, lOut, rOut);
}

template <>
void sfz::writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    writeInterleaved<float, false>(inputLeft, inputRight, output);
}

template <>
void sfz::fill<float, true>(absl::Span<float> output, float value) noexcept
{
    fill<float, false>(output, value);
}

template <>
void sfz::exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    exp<float, false>(input, output);
}

template <>
void sfz::log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    log<float, false>(input, output);
}

template <>
void sfz::sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    sin<float, false>(input, output);
}

template <>
void sfz::cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    cos<float, false>(input, output);
}

template <>
void sfz::applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
void sfz::applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
void sfz::divide<float, true>(absl::Span<const float> input, absl::Span<const float> divisor, absl::Span<float> output) noexcept
{
    divide<float, false>(input, divisor, output);
}

template <>
void sfz::multiplyAdd<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    multiplyAdd<float, false>(gain, input, output);
}

template <>
float sfz::loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept
{
    return loopingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd, loopStart);
}

template <>
float sfz::saturatingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd) noexcept
{
    return saturatingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd);
}


template <>
float sfz::linearRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return linearRamp<float, false>(output, start, step);
}

template <>
float sfz::multiplicativeRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return multiplicativeRamp<float, false>(output, start, step);
}

template <>
void sfz::add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    add<float, false>(input, output);
}

template <>
void sfz::add<float, true>(float value, absl::Span<float> output) noexcept
{
    add<float, false>(value, output);
}

template <>
void sfz::subtract<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    subtract<float, false>(input, output);
}

template <>
void sfz::subtract<float, true>(const float value, absl::Span<float> output) noexcept
{
    subtract<float, false>(value, output);
}


template <>
void sfz::copy<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    copy<float, false>(input, output);
}

template <>
void sfz::pan<float, true>(absl::Span<const float> panEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept
{
    pan<float, false>(panEnvelope, leftBuffer, rightBuffer);
}

template <>
float sfz::mean<float, true>(absl::Span<const float> vector) noexcept
{
    return mean<float, false>(vector);
}

template <>
float sfz::meanSquared<float, true>(absl::Span<const float> vector) noexcept
{
    return meanSquared<float, false>(vector);
}

template <>
void sfz::cumsum<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    cumsum<float, false>(input, output);
}

template<>
void sfz::sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> leftCoeffs, absl::Span<float> rightCoeffs) noexcept
{
    sfzInterpolationCast<float, false>(floatJumps, jumps, leftCoeffs, rightCoeffs);
}

template <>
void sfz::diff<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    diff<float, false>(input, output);
}
