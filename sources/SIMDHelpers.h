
#include "absl/types/span.h"
#include "Globals.h"
#include "Helpers.h"
#include <cmath>

template<class T, bool SIMD=SIMDConfig::readInterleaved>
void readInterleaved(absl::Span<const T> input, absl::Span<T> outputLeft, absl::Span<T> outputRight) noexcept
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
void writeInterleaved(absl::Span<const T> inputLeft, absl::Span<const T> inputRight, absl::Span<T> output) noexcept
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
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept;
template<>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept;

template<class T, bool SIMD=SIMDConfig::fill>
void fill(absl::Span<T> output, T value) noexcept
{
    std::fill(output.begin(), output.end(), value);
}

template<>
void fill<float, true>(absl::Span<float> output, float value) noexcept;

template<class Type, bool SIMD=SIMDConfig::useSIMD>
void exp(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::exp(input[i]);
}

template<>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template<class T, bool SIMD=SIMDConfig::useSIMD>
void loopingSFZIndex(absl::Span<const T> inputLeft, absl::Span<const T> inputRight, absl::Span<T> output);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void linearRamp(absl::Span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void exponentialRamp(absl::Span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void applyGain(T gain, absl::Span<T> output);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void applyGain(absl::Span<const T> gain, absl::Span<T> output);

