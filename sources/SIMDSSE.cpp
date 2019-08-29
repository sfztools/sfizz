#include "Helpers.h"
#include "SIMDHelpers.h"

#if HAVE_X86INTRIN_H
#include <x86intrin.h>
#endif

#if HAVE_INTRIN_H
#include <intrin.h>
#endif

#include "mathfuns/sse_mathfun.h"

using Type = float;
[[maybe_unused]] constexpr uintptr_t TypeAlignment { 4 };
[[maybe_unused]] constexpr uintptr_t TypeAlignmentMask { TypeAlignment - 1 };
[[maybe_unused]] constexpr uintptr_t ByteAlignment { TypeAlignment * sizeof(Type) };
[[maybe_unused]] constexpr uintptr_t ByteAlignmentMask { ByteAlignment - 1 };

struct AlignmentSentinels {
    float* nextAligned;
    float* lastAligned;
};

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

bool unaligned(const float* ptr1, const float* ptr2)
{
    return unaligned(ptr1) || unaligned(ptr2);
}

bool unaligned(const float* ptr1, const float* ptr2, const float* ptr3)
{
    return unaligned(ptr1) || unaligned(ptr2) || unaligned(ptr3);
}

bool unaligned(const float* ptr1, const float* ptr2, const float* ptr3, const float* ptr4)
{
    return unaligned(ptr1) || unaligned(ptr2) || unaligned(ptr3) || unaligned(ptr4);
}

template <>
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

    const auto size = std::min(input.size(), std::min(outputLeft.size() * 2, outputRight.size() * 2));
    const auto* lastAligned = prevAligned(input.begin() + size - TypeAlignment);

    while (unaligned(in, lOut, rOut) && in < lastAligned)
        snippetRead<float>(in, lOut, rOut);

    while (in < lastAligned) {
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
        snippetRead<float>(in, lOut, rOut);
}

template <>
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
        snippetWrite<float>(out, lIn, rIn);

    while (out < lastAligned) {
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
        snippetWrite<float>(out, lIn, rIn);
}

template <>
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

template <>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && in < lastAligned)
        *out++ = std::exp(*in++);

    while (in < lastAligned) {
        _mm_store_ps(out, exp_ps(_mm_load_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }

    while (in < sentinel)
        *out++ = std::exp(*in++);
}

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && in < lastAligned)
        *out++ = std::exp(*in++);

    while (in < lastAligned) {
        _mm_store_ps(out, cos_ps(_mm_load_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }

    while (in < sentinel)
        *out++ = std::exp(*in++);
}

template <>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && in < lastAligned)
        *out++ = std::exp(*in++);

    while (in < lastAligned) {
        _mm_store_ps(out, log_ps(_mm_load_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }

    while (in < sentinel)
        *out++ = std::exp(*in++);
}

template <>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = in + std::min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && in < lastAligned)
        *out++ = std::exp(*in++);

    while (in < lastAligned) {
        _mm_store_ps(out, sin_ps(_mm_load_ps(in)));
        out += TypeAlignment;
        in += TypeAlignment;
    }

    while (in < sentinel)
        *out++ = std::exp(*in++);
}

template <>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    const auto size = std::min(output.size(), input.size());
    const auto* lastAligned = prevAligned(output.begin() + size);
    const auto mmGain = _mm_set_ps1(gain);

    while (unaligned(out, in) && out < lastAligned)
        *out++ = gain * (*in++);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_mul_ps(mmGain, _mm_load_ps(in)));
        in += TypeAlignment;
        out += TypeAlignment;
    }

    while (out < output.end())
        *out++ = gain * (*in++);
}

template <>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    auto* g = gain.begin();
    const auto size = std::min(output.size(), std::min(input.size(), gain.size()));
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in, g) && out < lastAligned)
        snippetGainSpan<float>(g, in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_mul_ps(_mm_load_ps(g), _mm_load_ps(in)));
        g += TypeAlignment;
        in += TypeAlignment;
        out += TypeAlignment;
    }

    while (out < output.end())
        snippetGainSpan<float>(g, in, out);
}

template <>
float loopingSFZIndex<float, true>(absl::Span<const float> jumps,
    absl::Span<float> leftCoeffs,
    absl::Span<float> rightCoeffs,
    absl::Span<int> indices,
    float floatIndex,
    float loopEnd,
    float loopStart) noexcept
{
    ASSERT(indices.size() >= jumps.size());
    ASSERT(indices.size() == leftCoeffs.size());
    ASSERT(indices.size() == rightCoeffs.size());

    auto index = indices.data();
    auto leftCoeff = leftCoeffs.data();
    auto rightCoeff = rightCoeffs.data();
    auto jump = jumps.data();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    const auto* sentinel = jumps.begin() + size;
    const auto* alignedEnd = prevAligned(sentinel);

    while (unaligned(reinterpret_cast<float*>(index), leftCoeff, rightCoeff, jump) && jump < alignedEnd)
        snippetLoopingIndex<float>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd, loopStart);

    auto mmFloatIndex = _mm_set_ps1(floatIndex);
    const auto mmJumpBack = _mm_set1_ps(loopEnd - loopStart);
    const auto mmLoopEnd = _mm_set1_ps(loopEnd);
    while (jump < alignedEnd) {
        auto mmOffset = _mm_load_ps(jump);
        mmOffset = _mm_add_ps(mmOffset, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOffset), 4)));
        mmOffset = _mm_add_ps(mmOffset, _mm_shuffle_ps(_mm_setzero_ps(), mmOffset, 0x40));

        mmFloatIndex = _mm_add_ps(mmFloatIndex, mmOffset);
        const auto mmCompared = _mm_cmpge_ps(mmFloatIndex, mmLoopEnd);
        auto mmLoopBack = _mm_sub_ps(mmFloatIndex, mmJumpBack);
        mmLoopBack = _mm_and_ps(mmCompared, mmLoopBack);
        mmFloatIndex = _mm_andnot_ps(mmCompared, mmFloatIndex);
        mmFloatIndex = _mm_add_ps(mmFloatIndex, mmLoopBack);

        auto mmIndices = _mm_cvtps_epi32(_mm_sub_ps(mmFloatIndex, _mm_set_ps1(0.4999999552965164184570312f)));
        _mm_store_si128(reinterpret_cast<__m128i*>(index), mmIndices);

        auto mmRight = _mm_sub_ps(mmFloatIndex, _mm_cvtepi32_ps(mmIndices));
        auto mmLeft = _mm_sub_ps(_mm_set_ps1(1.0f), mmRight);
        _mm_store_ps(leftCoeff, mmLeft);
        _mm_store_ps(rightCoeff, mmRight);

        mmFloatIndex = _mm_shuffle_ps(mmFloatIndex, mmFloatIndex, _MM_SHUFFLE(3, 3, 3, 3));
        // floatingIndex = _mm_cvtss_f32(_mm_shuffle_ps(mmFloatIndex, mmFloatIndex, _MM_SHUFFLE(0, 0, 0, 3)));;
        // floatingIndex = *(index + 3) + *(rightCoeff + 3);
        index += TypeAlignment;
        jump += TypeAlignment;
        leftCoeff += TypeAlignment;
        rightCoeff += TypeAlignment;
    }

    floatIndex = _mm_cvtss_f32(mmFloatIndex);
    while (jump < sentinel)
        snippetLoopingIndex<float>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd, loopStart);
    return floatIndex;
}

template <>
float saturatingSFZIndex<float, true>(absl::Span<const float> jumps,
    absl::Span<float> leftCoeffs,
    absl::Span<float> rightCoeffs,
    absl::Span<int> indices,
    float floatIndex,
    float loopEnd) noexcept
{
    ASSERT(indices.size() >= jumps.size());
    ASSERT(indices.size() == leftCoeffs.size());
    ASSERT(indices.size() == rightCoeffs.size());

    auto index = indices.data();
    auto leftCoeff = leftCoeffs.data();
    auto rightCoeff = rightCoeffs.data();
    auto jump = jumps.data();
    const auto size = min(jumps.size(), indices.size(), leftCoeffs.size(), rightCoeffs.size());
    const auto* sentinel = jumps.begin() + size;
    const auto* alignedEnd = prevAligned(sentinel);

    while (unaligned(reinterpret_cast<float*>(index), leftCoeff, rightCoeff, jump) && jump < alignedEnd)
        snippetSaturatingIndex<float>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd);

    auto mmFloatIndex = _mm_set_ps1(floatIndex);
    const auto mmLoopEnd = _mm_set1_ps(loopEnd);
    const auto mmSaturated = _mm_sub_ps(mmLoopEnd, _mm_set_ps1(0.000001f));
    while (jump < alignedEnd) {
        auto mmOffset = _mm_load_ps(jump);
        mmOffset = _mm_add_ps(mmOffset, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOffset), 4)));
        mmOffset = _mm_add_ps(mmOffset, _mm_shuffle_ps(_mm_setzero_ps(), mmOffset, 0x40));

        mmFloatIndex = _mm_add_ps(mmFloatIndex, mmOffset);
        const auto mmCompared = _mm_cmplt_ps(mmFloatIndex, mmLoopEnd);
        mmFloatIndex = _mm_add_ps(_mm_and_ps(mmCompared, mmFloatIndex), _mm_andnot_ps(mmCompared, mmSaturated));

        auto mmIndices = _mm_cvtps_epi32(_mm_sub_ps(mmFloatIndex, _mm_set_ps1(0.4999999552965164184570312f)));
        _mm_store_si128(reinterpret_cast<__m128i*>(index), mmIndices);

        auto mmRight = _mm_sub_ps(mmFloatIndex, _mm_cvtepi32_ps(mmIndices));
        auto mmLeft = _mm_sub_ps(_mm_set_ps1(1.0f), mmRight);
        _mm_store_ps(leftCoeff, mmLeft);
        _mm_store_ps(rightCoeff, mmRight);

        mmFloatIndex = _mm_shuffle_ps(mmFloatIndex, mmFloatIndex, _MM_SHUFFLE(3, 3, 3, 3));
        // floatingIndex = _mm_cvtss_f32(_mm_shuffle_ps(mmFloatIndex, mmFloatIndex, _MM_SHUFFLE(0, 0, 0, 3)));;
        // floatingIndex = *(index + 3) + *(rightCoeff + 3);
        index += TypeAlignment;
        jump += TypeAlignment;
        leftCoeff += TypeAlignment;
        rightCoeff += TypeAlignment;
    }

    floatIndex = _mm_cvtss_f32(mmFloatIndex);
    while (jump < sentinel)
        snippetSaturatingIndex<float>(jump, leftCoeff, rightCoeff, index, floatIndex, loopEnd);
    return floatIndex;
}

template <>
float linearRamp<float, true>(absl::Span<float> output, float value, float step) noexcept
{
    auto* out = output.begin();
    const auto* lastAligned = prevAligned(output.end());

    while (unaligned(out) && out < lastAligned)
        snippetRampLinear<float>(out, value, step);

    auto mmValue = _mm_set1_ps(value);
    auto mmStep = _mm_set_ps(step + step + step + step, step + step + step, step + step, step);

    while (out < lastAligned) {
        mmValue = _mm_add_ps(mmValue, mmStep);
        _mm_store_ps(out, mmValue);
        mmValue = _mm_shuffle_ps(mmValue, mmValue, _MM_SHUFFLE(3, 3, 3, 3));
        out += TypeAlignment;
    }

    value = _mm_cvtss_f32(mmValue);
    while (out < output.end())
        snippetRampLinear<float>(out, value, step);
    return value;
}

template <>
float multiplicativeRamp<float, true>(absl::Span<float> output, float value, float step) noexcept
{
    auto* out = output.begin();
    const auto* lastAligned = prevAligned(output.end());

    while (unaligned(out) && out < lastAligned)
        snippetRampMultiplicative<float>(out, value, step);

    auto mmValue = _mm_set1_ps(value);
    auto mmStep = _mm_set_ps(step * step * step * step, step * step * step, step * step, step);

    while (out < lastAligned) {
        mmValue = _mm_mul_ps(mmValue, mmStep);
        _mm_store_ps(out, mmValue);
        mmValue = _mm_shuffle_ps(mmValue, mmValue, _MM_SHUFFLE(3, 3, 3, 3));
        out += TypeAlignment;
    }

    value = _mm_cvtss_f32(mmValue);
    while (out < output.end())
        snippetRampMultiplicative<float>(out, value, step);
    return value;
}

template <>
void add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    ASSERT(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && out < lastAligned)
        snippetAdd<float>(in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_add_ps(_mm_load_ps(in), _mm_load_ps(out)));
        out += TypeAlignment;
        in += TypeAlignment;
    }

    while (out < sentinel)
        snippetAdd<float>(in, out);
}