#pragma once
#include "Buffer.h"
#include "Helpers.h"
#include "Globals.h"

template<class Type, unsigned int NumChannels = sfz::Config::numChannels, unsigned int Alignment = sfz::Config::defaultAlignment>

class AudioBuffer
{
public:
    AudioBuffer() = default;
    AudioBuffer(int numFrames)
    {
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
        return *(buffer.data() + numFrames * channelIndex + sampleIndex);
    }

    Type* getChannel(int channelIndex) noexcept
    {
        return channels[channelIndex];
    }

    Type* begin(int channelIndex) noexcept
    {
        return buffer.data() + numFrames * channelIndex;
    }

    Type* end(int channelIndex) noexcept
    {
        return buffer.data() + numFrames * (channelIndex + 1);
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
};

template<class Type, unsigned int NumChannels = sfz::Config::numChannels, unsigned int Alignment = sfz::Config::defaultAlignment>
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

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return NumChannels; }
    bool empty() const noexcept { return numFrames == 0; }
private:
    int numFrames { 0 };
    std::array<Buffer<Type, Alignment>, NumChannels> buffers;
};