#include "Globals.h"
#include <absl/types/span.h>
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

template<class Type, bool SIMD=SIMDConfig::mathfuns>
void exp(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::exp(input[i]);
}

template<>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template<class Type, bool SIMD=SIMDConfig::mathfuns>
void log(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::log(input[i]);
}

template<>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template<class Type, bool SIMD=SIMDConfig::mathfuns>
void sin(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::sin(input[i]);
}

template<>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template<class Type, bool SIMD=SIMDConfig::mathfuns>
void cos(absl::Span<const Type> input, absl::Span<Type> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto sentinel = std::min(input.size(), output.size());
    for (decltype(sentinel) i = 0; i < sentinel; ++i)
        output[i] = std::cos(input[i]);
}

template<>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept;

template<class T, bool SIMD=SIMDConfig::useSIMD>
void loopingSFZIndex(absl::Span<const T> jumps, absl::Span<T> leftCoeffs, absl::Span<T> rightCoeffs, absl::Span<int> indices, T floatIndex, T loopEnd, T loopStart) noexcept
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
}

template<>
void loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept;

template<class T, bool SIMD=SIMDConfig::useSIMD>
void linearRamp(absl::Span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::useSIMD>
void exponentialRamp(absl::Span<T> output, T start, T end);

template<class T, bool SIMD=SIMDConfig::gain>
void applyGain(T gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(input.size() <= output.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(output.size(), input.size());
    while (out < sentinel)
    {
        *out++ = gain * (*in++);
    }
}

template<class T, bool SIMD=SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<const T> input, absl::Span<T> output) noexcept
{
    ASSERT(gain.size() == input.size());
    ASSERT(input.size() <= output.size());
    auto* in = input.begin();
    auto* g = gain.begin();
    auto* out = output.begin();
    auto* sentinel = out + std::min(gain.size(), std::min(output.size(), input.size()));
    while (out < sentinel)
    {
        *out++ = (*g++) * (*in++);
    }
}

template<class T, bool SIMD=SIMDConfig::gain>
void applyGain(T gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

template<class T, bool SIMD=SIMDConfig::gain>
void applyGain(absl::Span<const T> gain, absl::Span<T> output) noexcept
{
    applyGain<T, SIMD>(gain, output, output);
}

template<>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept;

template<>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept;