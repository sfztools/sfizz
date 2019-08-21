#include "SIMDHelpers.h"
#include "Helpers.h"
#include "x86intrin.h"
#include "mathfuns/sse_mathfun.h"

constexpr int TypeAlignment { 4 };
using Type = float;

template<>
void readInterleaved<Type, true>(absl::Span<const Type> input, absl::Span<Type> outputLeft, absl::Span<Type> outputRight) noexcept
{
    // The size of the outputs is not big enough for the input...
    ASSERT(outputLeft.size() >= input.size() / 2);
    ASSERT(outputRight.size() >= input.size() / 2);
    // Input is too small
    ASSERT(input.size() > 1);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();
    const int unalignedEnd = input.size() & (2 * TypeAlignment - 1);
    const int lastAligned = input.size() - unalignedEnd;
    auto* inputSentinel = in + lastAligned;
    while (in < inputSentinel && lOut < outputLeft.end() && rOut < outputRight.end())
    {
        auto register0 = _mm_loadu_ps(in);
        in += TypeAlignment;
        auto register1 = _mm_loadu_ps(in);
        in += TypeAlignment;
        auto register2 = register0;
        // register 2 holds the copy of register 0 that is going to get erased by the first operation
        // Remember that the bit mask reads from the end; 10 00 10 00 means
        // "take 0 from a, take 2 from a, take 0 from b, take 2 from b"
        register0 = _mm_shuffle_ps(register0, register1, 0b10001000);
        register1 = _mm_shuffle_ps(register2, register1, 0b11011101);
        _mm_storeu_ps(lOut, register0);
        _mm_storeu_ps(rOut, register1);
        lOut += TypeAlignment;
        rOut += TypeAlignment;
    }
    
    inputSentinel = input.end() - 1;
    while (in < inputSentinel && lOut < outputLeft.end() && rOut < outputRight.end())
    {
        *lOut++ = *in++;
        *rOut++ = *in++;
    }
}

template<>
void writeInterleaved<Type, true>(absl::Span<const Type> inputLeft, absl::Span<const Type> inputRight, absl::Span<Type> output) noexcept
{
    // The size of the output is not big enough for the inputs...
    ASSERT(inputLeft.size() <= output.size() / 2);
    ASSERT(inputRight.size() <= output.size() / 2);

    auto* lIn = inputLeft.begin();
    auto* rIn = inputRight.begin();
    auto* out = output.begin();

    const int residualLeft = inputLeft.size() & (TypeAlignment - 1);
    const int residualRight = inputRight.size() & (TypeAlignment - 1);
    const auto* leftSentinel = lIn + inputLeft.size() - residualLeft;
    const auto* rightSentinel = rIn + inputRight.size() - residualRight;
    const auto* outputSentinel = output.end() - 1;

    while (lIn < leftSentinel && rIn < rightSentinel && out < outputSentinel)
    {
        const auto lInRegister = _mm_loadu_ps(lIn);
        const auto rInRegister = _mm_loadu_ps(rIn);

        const auto outRegister1 = _mm_unpacklo_ps(lInRegister, rInRegister);
        _mm_storeu_ps(out, outRegister1);
        out += TypeAlignment;

        const auto outRegister2 = _mm_unpackhi_ps(lInRegister, rInRegister);
        _mm_storeu_ps(out, outRegister2);
        out += TypeAlignment;

        lIn += TypeAlignment;
        rIn += TypeAlignment;
    }

    while (lIn < inputLeft.end() && rIn < inputRight.end() && out < outputSentinel)
    {

        *out++ = *lIn++;
        *out++ = *rIn++;
    }
}

template<>
void fill<float, true>(absl::Span<float> output, float value) noexcept
{
    const auto mmValue = _mm_set_ps1(value);
    auto* out = output.begin();
    const int residual = output.size() & (TypeAlignment - 1);
    const auto* sentinel = output.end() - residual;
    
    while (out < sentinel) // we should only need to test a single channel
    {
        _mm_storeu_ps(out, mmValue);
        out += TypeAlignment;
    }
    
    while (out < output.end())
        *out++ = value;
}

template<>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    while (in < sentinel)
    {
        _mm_storeu_ps(out, exp_ps(_mm_loadu_ps(in)));
        out += 4;
        in += 4;
    }
}