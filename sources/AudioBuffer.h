#pragma once
#include "Buffer.h"
#include "Helpers.h"
#include "Globals.h"
#include <array>
#include <iostream>
#include <type_traits>

template<class Type, unsigned int NumChannels = sfz::config::numChannels, unsigned int Alignment = config::defaultAlignment>

class AudioBuffer
{
public:
    AudioBuffer() = default;
    AudioBuffer(int numFrames)
    {
        DBG("Building an audiobuffer of size " << numFrames);
        resize(numFrames);
    }
    
    bool resize(int numFrames) 
    {
        // should have a positive number of frames...
        ASSERT(numFrames >= 0);
        padding = TypeAlignment - (numFrames & TypeAlignmentMask);
        totalSize = NumChannels * (numFrames + padding);
        if (buffer.resize(static_cast<size_t>(totalSize)))
        {
            this->numFrames = numFrames;
            for (auto channelIndex = 0; channelIndex < NumChannels; ++channelIndex)
                channels[channelIndex] = buffer.data() + channelIndex * (numFrames + padding);
            return true;
        }

        return false;
    }

    Type& getSample(int channelIndex, int sampleIndex) noexcept
    {
        ASSERT(channelIndex >= 0);
        ASSERT(sampleIndex >= 0);
        return *(channels[channelIndex] + sampleIndex);
    }

    template<VectorOperations op = VectorOperations::standard>
    void fill(Type value) noexcept
    {
        if constexpr (op == VectorOperations::sse && std::is_same<Type, float>::value)
        {
            static_assert(Alignment == 16, "Wrong alignment");
            const __m128 mmValue = _mm_set_ps1(value);
            for(auto i = 0; i < NumChannels; ++i)
                for (auto mm = (__m128*)alignedBegin(i); mm < (__m128*)alignedEnd(i); mm++)
                    _mm_store_ps((float*)mm, mmValue);
        }
        else
        {
            for(auto i = 0; i < NumChannels; ++i)
                std::fill(begin(i), end(i), value);
        }
    }

    template<VectorOperations op = VectorOperations::standard>
    void readInterleaved(float* input, int numFrames) noexcept
    {
        ASSERT(this->numFrames >= numFrames);
        if constexpr (op == VectorOperations::sse && std::is_same<Type, float>::value && NumChannels == 2)
        {
            static_assert(Alignment == 16, "Wrong alignment");
            const int residualFrames = numFrames & (2 * TypeAlignment - 1);
            const int lastAligned = numFrames - residualFrames;
            float* in = input;
            float* out0 = getChannel(0);
            float* out1 = getChannel(1);
            const float* end = input + 2 * lastAligned;
            while (in < end)
            {
                const auto input0 = _mm_loadu_ps(in);
                in += 4;
                const auto input1 = _mm_loadu_ps(in);
                in += 4;
                const auto intermediate0 = _mm_unpacklo_ps(input0, input1);
                const auto intermediate1 = _mm_unpackhi_ps(input0, input1);
                const auto output0 = _mm_unpacklo_ps(intermediate0, intermediate1);
                const auto output1 = _mm_unpackhi_ps(intermediate0, intermediate1);
                _mm_store_ps(out0, output0);
                _mm_store_ps(out1, output1);
                out0 += 4;
                out1 += 4;
            }
            for (auto chanIdx = 0; chanIdx < NumChannels; chanIdx++)
            {
                auto* _in = input + 2 * lastAligned + chanIdx;
                auto* _end = input + numFrames * NumChannels;
                auto* _out = getChannel(chanIdx) + lastAligned;
                while (_in < _end)
                {
                    *_out = *_in;
                    _in += 2;
                    _out += 1;
                }
            }
        }
        else
        {
            for (auto chanIdx = 0; chanIdx < NumChannels; chanIdx++)
            {
                auto* _in = input + chanIdx;
                auto* _end = input + numFrames * NumChannels;
                auto* _out = getChannel(chanIdx);
                while (_in < _end)
                {
                    *_out = *_in;
                    _in += NumChannels;
                    _out += 1;
                }
            }
        }
    }

    Type* getChannel(int channelIndex) noexcept
    {
        return channels[channelIndex];
    }

    Type* begin(int channelIndex) noexcept
    {
        return channels[channelIndex];
    }

    Type* end(int channelIndex) noexcept
    {
        return buffer.data() + numFrames * (channelIndex + 1) + padding * channelIndex;
    }

    Type* alignedBegin(int channelIndex) noexcept
    {
        return begin(channelIndex);
    }

    Type* alignedEnd(int channelIndex) noexcept
    {
        return buffer.data() + (channelIndex + 1) * (numFrames + padding);
    }

    Type& operator()(int channelIndex, int sampleIndex) noexcept
    {
        return getSample(channelIndex, sampleIndex);
    }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return NumChannels; }
    bool empty() const noexcept { return numFrames == 0; }

private:
    static constexpr auto TypeAlignment { Alignment / sizeof(Type) };
    static constexpr auto TypeAlignmentMask { TypeAlignment - 1 };
    static_assert(TypeAlignment * sizeof(Type) == Alignment, "The alignment does not appear to be divided by the size of the Type");
    int numFrames { 0 };
    int totalSize { 0 };
    int padding { 0 };
    std::array<Type*, NumChannels> channels;
    Buffer<Type, Alignment> buffer {};
    Buffer<Type, Alignment> tempBuffer {2 * Alignment};
};

template<class Type, unsigned int NumChannels = sfz::config::numChannels, unsigned int Alignment = config::defaultAlignment>
class SplitAudioBuffer
{
public:
    SplitAudioBuffer() = default;
    SplitAudioBuffer(int numFrames)
    {
        resize(numFrames);
    }

    bool resize(int numFrames)
    {
        // should have a positive number of frames...
        ASSERT(numFrames >= 0);
        bool resizedOK = true;

        for (auto& buffer: buffers)
            resizedOK &= buffer.resize(static_cast<size_t>(numFrames));

        if (resizedOK)
            this->numFrames = numFrames;
        else
            this->numFrames = std::min(numFrames, this->numFrames);

        return resizedOK;
    }

    template<VectorOperations op = VectorOperations::standard>
    void fill(Type value) noexcept
    {
        if constexpr (op == VectorOperations::sse)
        {
            static_assert(Alignment == 16, "Wrong alignment");
            const __m128 mmValue = _mm_set_ps1(value);
            for(auto i = 0; i < NumChannels; ++i)
                for (auto mm = (__m128*)alignedBegin(i); mm < (__m128*)alignedEnd(i); mm++)
                    _mm_store_ps((float*)mm, mmValue);
        }
        else
        {
            for(auto i = 0; i < NumChannels; ++i)
                std::fill(begin(i), end(i), value);
        }
    }

    Type& getSample(int channelIndex, int sampleIndex) noexcept
    {
        ASSERT(channelIndex >= 0);
        ASSERT(sampleIndex >= 0);
        return *(buffers[channelIndex].data() + sampleIndex);
    }

    Type& operator()(int channelIndex, int sampleIndex) noexcept
    {
        return getSample(channelIndex, sampleIndex);
    }

    Type* getChannel(int channelIndex) noexcept
    {
        return buffers[channelIndex].data();
    }

    Type* begin(int channelIndex) noexcept
    {
        return buffers[channelIndex].begin();
    }

    Type* end(int channelIndex) noexcept
    {
        return buffers[channelIndex].end();
    }

    Type* alignedBegin(int channelIndex) noexcept
    {
        return begin(channelIndex);
    }

    Type* alignedEnd(int channelIndex) noexcept
    {
        return buffers[channelIndex].alignedEnd();
    }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return NumChannels; }
    bool empty() const noexcept { return numFrames == 0; }
private:
    int numFrames { 0 };
    std::array<Buffer<Type, Alignment>, NumChannels> buffers;
};