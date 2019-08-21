#include "SIMDHelpers.h"
#include "Helpers.h"
#include "x86intrin.h"
#include "mathfuns/sse_mathfun.h"

constexpr uintptr_t TypeAlignment { 4 };
constexpr uintptr_t TypeAlignmentMask { TypeAlignment - 1 };
using Type = float;
constexpr uintptr_t ByteAlignment { TypeAlignment * sizeof(Type) };
constexpr uintptr_t ByteAlignmentMask { ByteAlignment - 1 };


struct AlignmentSentinels { float* nextAligned; float* lastAligned; };

float* nextAligned(const float* ptr)
{
    return reinterpret_cast<float*>( (reinterpret_cast<uintptr_t>(ptr) + ByteAlignmentMask) & (~ByteAlignmentMask) );
}

float* prevAligned(const float* ptr)
{
    return reinterpret_cast<float*>( reinterpret_cast<uintptr_t>(ptr) & (~ByteAlignmentMask) );
}

bool unaligned(const float* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & ByteAlignmentMask) != 0;
}

bool unaligned(const float* ptr1, const float* ptr2)
{
    return unaligned(ptr1) || unaligned(ptr2);
}

bool unaligned(const float* ptr1, const float* ptr2, const float* ptr3)
{
    return unaligned(ptr1) || unaligned(ptr2) || unaligned(ptr3);
}

template<>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    // The size of the outputs is not big enough for the input...
    ASSERT(outputLeft.size() >= input.size() / 2);
    ASSERT(outputRight.size() >= input.size() / 2);
    // Input is too small
    ASSERT(input.size() > 1);

    auto* in = input.begin();
    auto* lOut = outputLeft.begin();
    auto* rOut = outputRight.begin();

    const auto size = std::min(input.size(), std::min(outputLeft.size() * 2, outputRight.size() * 2 ));
    const auto* lastAligned = prevAligned(input.begin() + size - TypeAlignment);

    while (unaligned(in, lOut, rOut) && in < lastAligned)
    {
        *lOut++ = *in++;
        *rOut++ = *in++;
    }

    while (in < lastAligned )
    {
        auto register0 = _mm_load_ps(in);
        in += TypeAlignment;
        auto register1 = _mm_load_ps(in);
        in += TypeAlignment;
        auto register2 = register0;
        // register 2 holds the copy of register 0 that is going to get erased by the first operation
        // Remember that the bit mask reads from the end; 10 00 10 00 means
        // "take 0 from a, take 2 from a, take 0 from b, take 2 from b"
        register0 = _mm_shuffle_ps(register0, register1, 0b10001000);
        register1 = _mm_shuffle_ps(register2, register1, 0b11011101);
        _mm_store_ps(lOut, register0);
        _mm_store_ps(rOut, register1);
        lOut += TypeAlignment;
        rOut += TypeAlignment;
    }
    
    while (in < input.end() - 1)
    {
        *lOut++ = *in++;
        *rOut++ = *in++;
    }
}

template<>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    // The size of the output is not big enough for the inputs...
    ASSERT(inputLeft.size() <= output.size() / 2);
    ASSERT(inputRight.size() <= output.size() / 2);

    auto* lIn = inputLeft.begin();
    auto* rIn = inputRight.begin();
    auto* out = output.begin();

    const auto size = std::min(output.size(), std::min(inputLeft.size(), inputRight.size()) * 2);
    const auto* lastAligned = prevAligned(output.begin() + size - TypeAlignment);

    while (unaligned(out, rIn, lIn) && out < lastAligned)
    {
        *out++ = *lIn++;
        *out++ = *rIn++;
    }

    while (out < lastAligned)
    {
        const auto lInRegister = _mm_load_ps(lIn);
        const auto rInRegister = _mm_load_ps(rIn);

        const auto outRegister1 = _mm_unpacklo_ps(lInRegister, rInRegister);
        _mm_store_ps(out, outRegister1);
        out += TypeAlignment;

        const auto outRegister2 = _mm_unpackhi_ps(lInRegister, rInRegister);
        _mm_store_ps(out, outRegister2);
        out += TypeAlignment;

        lIn += TypeAlignment;
        rIn += TypeAlignment;
    }

    while (out < output.end() - 1)
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
    const auto* lastAligned = prevAligned(output.end());
    
    while (unaligned(out) && out < lastAligned)
        *out++ = value;

    while (out < lastAligned) // we should only need to test a single channel
    {
        _mm_store_ps(out, mmValue);
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
        out += TypeAlignment;
        in += TypeAlignment;
    }
}

template<>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    while (in < sentinel)
    {
        _mm_storeu_ps(out, cos_ps(_mm_loadu_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }
}

template<>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    while (in < sentinel)
    {
        _mm_storeu_ps(out, log_ps(_mm_loadu_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }
}



template<>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    while (in < sentinel)
    {
        _mm_storeu_ps(out, sin_ps(_mm_loadu_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }
}

template<>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    const auto size = std::min(output.size(), input.size());
    const auto* lastAligned = prevAligned(output.begin() + size);
    const auto mmGain = _mm_set_ps1(gain);

    while (unaligned(out, in) && out < lastAligned)
        *out++ = gain * (*in++);
    
    while (out < lastAligned)
    {
        _mm_store_ps(out, _mm_mul_ps(mmGain, _mm_load_ps(in)));
        in += TypeAlignment;
        out += TypeAlignment;
    }

    while (out < output.end())
        *out++ = gain * (*in++);
}

template<>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    auto* g = gain.begin();
    const auto size = std::min(output.size(), std::min(input.size(), gain.size()));
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in, g) && out < lastAligned)
        *out++ = (*g++) * (*in++);

    while (out < lastAligned)
    {
        _mm_store_ps(out, _mm_mul_ps(_mm_load_ps(g), _mm_load_ps(in)));
        g += TypeAlignment;
        in += TypeAlignment;
        out += TypeAlignment;
    }

    while (out < output.end())
        *out++ = (*g++) * (*in++);
}