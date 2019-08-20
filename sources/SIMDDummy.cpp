#include "SIMDHelpers.h"
#include "Helpers.h"

template<>
void readInterleaved<float, true>(gsl::span<const float> input, gsl::span<float> outputLeft, gsl::span<float> outputRight) noexcept
{
    readInterleaved<float, false>(input, outputLeft, outputRight);
}

template<>
void writeInterleaved<float, true>(gsl::span<const float> inputLeft, gsl::span<const float> inputRight, gsl::span<float> output) noexcept
{
    writeInterleaved<float, false>(inputLeft, inputRight, output);
}

// template<class Type, bool SIMD=false>
// void loopingSFZIndex(gsl::span<const Type> inputLeft, gsl::span<const Type> inputRight, gsl::span<Type> output);

// template<class Type, bool SIMD=false>
// void linearRamp(gsl::span<Type> output, Type start, Type end);

// template<class Type, bool SIMD=false>
// void exponentialRamp(gsl::span<Type> output, Type start, Type end);

// template<class Type, bool SIMD=false>
// void applyGain(Type gain, gsl::span<Type> output);

// template<class Type, bool SIMD=false>
// void applyGain(gsl::span<const Type> output, gsl::span<Type> output);

template<>
void fill<float, true>(gsl::span<float> output, float value) noexcept
{
    fill<float, false>(output, value);
}