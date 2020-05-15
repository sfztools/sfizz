// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDConfig.h"

#if !(SFIZZ_HAVE_SSE2 || SFIZZ_HAVE_NEON)

#include "SIMDHelpers.h"

template <>
void sfz::readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    readInterleaved<float, false>(input, outputLeft, outputRight);
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
void sfz::multiplyAdd<float, true>(const float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
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
void sfz::sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> coeffs) noexcept
{
    sfzInterpolationCast<float, false>(floatJumps, jumps, coeffs);
}

template <>
void sfz::diff<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    diff<float, false>(input, output);
}

#endif // !(SFIZZ_HAVE_SSE2 || SFIZZ_HAVE_NEON)
