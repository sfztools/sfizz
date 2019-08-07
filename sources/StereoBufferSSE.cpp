#include "StereoBuffer.h"
#include "Intrinsics.h"

template <>
template <>
void StereoBuffer<float, 16>::readInterleaved<true>(float *input, int numFrames) noexcept
{
    // The number of frames to read has to fit!
    ASSERT(this->numFrames >= numFrames);
    const int residualFrames = numFrames & (2 * TypeAlignment - 1);
    const int lastAligned = numFrames - residualFrames;
    float *in = input;
    auto [lOut, rOut] = getChannels();
    const float *end = input + 2 * lastAligned;
    while (in < end)
    {
        auto register0 = _mm_loadu_ps(in);
        in += 4;
        auto register1 = _mm_loadu_ps(in);
        in += 4;
        auto register2 = register0;
        // register 2 holds the copy of register 0 that is going to get erased by the first operation
        // Remember that the bit mask reads from the end; 10 00 10 00 means
        // "take 0 from a, take 2 from a, take 0 from b, take 2 from b"
        register0 = _mm_shuffle_ps(register0, register1, 0b10001000);
        register1 = _mm_shuffle_ps(register2, register1, 0b11011101);
        _mm_store_ps(lOut, register0);
        _mm_store_ps(rOut, register1);
        lOut += 4;
        rOut += 4;
    }
    end = input + numChannels * numFrames;
    while (in < end)
    {
        *lOut = *in;
        in++;
        *rOut = *in;
        in++;
        lOut += 1;
        rOut += 1;
    }
}

template<>
template<>
void StereoBuffer<float, 16>::fill<true>(float value) noexcept
{
    const __m128 mmValue = _mm_set_ps1(value);
    auto [lBegin, rBegin] = getChannels();
    auto [lEnd, rEnd] = alignedEnds();
    // Cast to m128 so that pointer arithmetics make sense
    auto mmLeft = reinterpret_cast<__m128*>(lBegin);
    auto mmRight = reinterpret_cast<__m128*>(rBegin);
    while (mmLeft < reinterpret_cast<__m128*>(lEnd)) // we should only need to test a single channel
    {
        _mm_store_ps(reinterpret_cast<float*>(mmLeft), mmValue);
        _mm_store_ps(reinterpret_cast<float*>(mmRight), mmValue);
        mmLeft++;
        mmRight++;
    }
}

template<>
template<>
void StereoBuffer<float, 16>::writeInterleaved<true>(float* output, int numFrames) noexcept
{
    ASSERT(numFrames <= this->numFrames);
    const int residualFrames = numFrames & (TypeAlignment - 1);
    const int lastAligned = numFrames - residualFrames;
    auto [lIn, rIn] = getChannels();
    auto* out = output;
    const auto* sseEnd = lIn + lastAligned;
    while (lIn < sseEnd)
    {
        const auto lInRegister = _mm_load_ps(lIn);
        const auto rInRegister = _mm_load_ps(rIn);
        
        const auto outRegister1 = _mm_unpacklo_ps(lInRegister, rInRegister);
        _mm_storeu_ps(out, outRegister1);
        out += TypeAlignment;
        const auto outRegister2 = _mm_unpackhi_ps(lInRegister, rInRegister);
        _mm_storeu_ps(out, outRegister2);
        out += TypeAlignment;

        lIn += TypeAlignment;
        rIn += TypeAlignment;
    }

    const auto* end = output + numChannels * numFrames;
    while (out < end)
    {
        *out++ = *lIn++;
        *out++ = *rIn++;
    }
}