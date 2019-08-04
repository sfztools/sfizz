#pragma once
#include "Buffer.h"
#include "Helpers.h"
#include "Globals.h"
#include <array>
#include <iostream>
#include <type_traits>

enum class Channel {left, right};

template<class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class StereoBuffer
{
public:
    static constexpr int numChannels { 2 };
    StereoBuffer() = default;
    StereoBuffer(int numFrames)
    {
        resize(numFrames);
    }
    
    bool resize(int numFrames) 
    {
        // should have a positive number of frames...
        ASSERT(numFrames >= 0);
        if (leftBuffer.resize(static_cast<size_t>(numFrames)) && rightBuffer.resize(static_cast<size_t>(numFrames)))
        {
            this->numFrames = numFrames;
            return true;
        }

        return false;
    }

    Type& getSample(Channel channel, int sampleIndex) noexcept
    {
        ASSERT(sampleIndex >= 0);
        switch(channel)
        {
        case Channel::left: return leftBuffer[sampleIndex];
        case Channel::right: return rightBuffer[sampleIndex];
        // Should not be here by construction...
        default: 
            ASSERTFALSE; 
            return trash;
        }
    }

    template<SIMD op = SIMD::scalar>
    void fill(Type value) noexcept
    {
        std::fill(begin(Channel::left), end(Channel::left), value);
        std::fill(begin(Channel::right), end(Channel::right), value);
    }

    template<SIMD op = SIMD::scalar>
    void readInterleaved(Type* input, int numFrames) noexcept
    {
        auto* in = input;
        auto* end = input + numChannels * numFrames;
        auto [lOut, rOut] = getChannels();
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

    template <>
    void readInterleaved<SIMD::sse>(float *input, int numFrames) noexcept
    {
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
    void fill<SIMD::sse>(float value) noexcept
    {
        const __m128 mmValue = _mm_set_ps1(value);
        auto [lBegin, rBegin] = getChannels();
        auto [lEnd, rEnd] = alignedEnds();
        auto mmLeft = reinterpret_cast<__m128 *>(lBegin);
        auto mmRight = reinterpret_cast<__m128 *>(rBegin);
        while (mmLeft < reinterpret_cast<__m128 *>(lEnd)) // we should only need to test a single channel
        {
            _mm_store_ps(reinterpret_cast<float *>(mmLeft), mmValue);
            _mm_store_ps(reinterpret_cast<float *>(mmRight), mmValue);
            mmLeft++;
            mmRight++;
        }
    }

    Type* getChannel(Channel channel) noexcept
    {
        switch(channel)
        {
        case Channel::left: return leftBuffer.data();
        case Channel::right: return rightBuffer.data();
        default: return {};
        }
    }

    Type* begin(Channel channel) noexcept
    {
        switch(channel)
        {
        case Channel::left: return leftBuffer.data();
        case Channel::right: return rightBuffer.data();
        default: return {};
        }
    }
    std::pair<Type*, Type*> getChannels() noexcept { return { leftBuffer.data(), rightBuffer.data() }; }
    std::pair<Type*, Type*> begins() noexcept { return { leftBuffer.data(), rightBuffer.data() }; }

    Type* end(Channel channel) noexcept
    {
        switch(channel)
        {
        case Channel::left: return leftBuffer.end();
        case Channel::right: return rightBuffer.end();
        default: return {};
        }
    }
    std::pair<Type*, Type*> ends() { return { leftBuffer.end(), rightBuffer.end() }; }

    Type* alignedEnd(Channel channel) noexcept
    {
        switch(channel)
        {
        case Channel::left: return leftBuffer.alignedEnd();
        case Channel::right: return rightBuffer.alignedEnd();
        default: return {};
        }
    }
    std::pair<Type*, Type*> alignedEnds() { return { leftBuffer.alignedEnd(), rightBuffer.alignedEnd() }; }

    Type& operator()(Channel channel, int sampleIndex) noexcept
    {
        return getSample(channel, sampleIndex);
    }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return numChannels; }
    bool empty() const noexcept { return numFrames == 0; }

private:
    static constexpr auto TypeAlignment { Alignment / sizeof(Type) };
    static constexpr auto TypeAlignmentMask { TypeAlignment - 1 };
    static_assert(TypeAlignment * sizeof(Type) == Alignment, "The alignment does not appear to be divided by the size of the Type");
    int numFrames { 0 };
    int totalSize { 0 };
    int padding { 0 };
    Buffer<Type, Alignment> leftBuffer {};
    Buffer<Type, Alignment> rightBuffer {};
    Type trash { 0 };
};
