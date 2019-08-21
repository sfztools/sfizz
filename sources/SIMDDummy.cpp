#include "SIMDHelpers.h"
#include "Helpers.h"

template<>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    readInterleaved<float, false>(input, outputLeft, outputRight);
}

template<>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    writeInterleaved<float, false>(inputLeft, inputRight, output);
}

// template<class Type, bool SIMD=false>
// void loopingSFZIndex(absl::Span<const Type> inputLeft, absl::Span<const Type> inputRight, absl::Span<Type> output);

// template<class Type, bool SIMD=false>
// void linearRamp(absl::Span<Type> output, Type start, Type end);

// template<class Type, bool SIMD=false>
// void exponentialRamp(absl::Span<Type> output, Type start, Type end);

// template<class Type, bool SIMD=false>
// void applyGain(Type gain, absl::Span<Type> output);

// template<class Type, bool SIMD=false>
// void applyGain(absl::Span<const Type> output, absl::Span<Type> output);

template<>
void fill<float, true>(absl::Span<float> output, float value) noexcept
{
    fill<float, false>(output, value);
}

template<>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    exp<float, false>(input, output);
}

template<>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    log<float, false>(input, output);
}

template<>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    sin<float, false>(input, output);
}

template<>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    cos<float, false>(input, output);
}