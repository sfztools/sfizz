#include "gsl/gsl-lite.hpp"
#include "Globals.h"
#include "Helpers.h"

template<class T, bool SIMD=SIMDConfig::readInterleaved>
void readInterleaved(gsl::span<const T> input, gsl::span<T> outputLeft, gsl::span<T> outputRight) noexcept
{
    // The size of the output is not big enough for the input...
    ASSERT(outputLeft.size() >= input.size() / 2);
    ASSERT(outputRight.size() >= input.size() / 2);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();
    while (in < (input.end() - 1) && lOut < outputLeft.end() && rOut < outputRight.end())
    {
        *lOut++ = *in++;
        *rOut++ = *in++;
    }
}

template<class T, bool SIMD=SIMDConfig::writeInterleaved>
void writeInterleaved(gsl::span<const T> inputLeft, gsl::span<const T> inputRight, gsl::span<T> output) noexcept
{
    ASSERT(inputLeft.size() <= output.size() / 2);
    ASSERT(inputRight.size() <= output.size() / 2);

    auto* lIn = inputLeft.begin();
    auto* rIn = inputRight.begin();
    auto* out = output.begin();
    while (lIn < inputLeft.end() && rIn < inputRight.end() && out < (output.end() - 1))
    {
        *out++ = *lIn++;
        *out++ = *rIn++;
    }
}

// Specializations
template<>
void writeInterleaved<float, true>(gsl::span<const float> inputLeft, gsl::span<const float> inputRight, gsl::span<float> output) noexcept;
template<>
void readInterleaved<float, true>(gsl::span<const float> input, gsl::span<float> outputLeft, gsl::span<float> outputRight) noexcept;

template<class T, bool SIMD=SIMDConfig::fill>
void fill(gsl::span<T> output, T value) noexcept
{
    std::fill(output.begin(), output.end(), value);
}

template<>
void fill<float, true>(gsl::span<float> output, float value) noexcept;

template<class T, bool SIMD=SIMDConfig::useSIMD>
void loopingSFZIndex(gsl::span<const T> inputLeft, gsl::span<const T> inputRight, gsl::span<T> output);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void linearRamp(gsl::span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void exponentialRamp(gsl::span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void applyGain(T gain, gsl::span<T> output);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void applyGain(gsl::span<const T> gain, gsl::span<T> output);

