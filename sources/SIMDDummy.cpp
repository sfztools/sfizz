#include "Helpers.h"
#include "SIMDHelpers.h"

template <>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    readInterleaved<float, false>(input, outputLeft, outputRight);
}

template <>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    writeInterleaved<float, false>(inputLeft, inputRight, output);
}

template <>
void fill<float, true>(absl::Span<float> output, float value) noexcept
{
    fill<float, false>(output, value);
}

template <>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    exp<float, false>(input, output);
}

template <>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    log<float, false>(input, output);
}

template <>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    sin<float, false>(input, output);
}

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    cos<float, false>(input, output);
}

template <>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
float loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept
{
    return loopingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd, loopStart);
}

template <>
float saturatingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd) noexcept
{
    return saturatingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd);
}


template <>
float linearRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return linearRamp<float, false>(output, start, step);
}

template <>
float multiplicativeRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return multiplicativeRamp<float, false>(output, start, step);
}

template <>
void add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    add<float, false>(input, output);
}