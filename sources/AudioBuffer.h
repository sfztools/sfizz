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
        if (buffer.resize(static_cast<size_t>(NumChannels * numFrames)))
        {
            this->numFrames = numFrames;
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
        ASSERT(channelIndex >= 0);
        if (channelIndex < NumChannels)
            return buffer.data() + numFrames * channelIndex;
        else
            return nullptr;
    }

    Type* begin(int channelIndex) noexcept
    {
        ASSERT(channelIndex >= 0);
        return buffer.data() + numFrames * channelIndex;
    }

    Type* end(int channelIndex) noexcept
    {
        ASSERT(channelIndex >= 0);
        return buffer.data() + numFrames * (channelIndex + 1);
    }

    // const Type* cbegin(int channelIndex) noexcept
    // {
    //     ASSERT(channelIndex >= 0);
    //     return buffer.data() + numFrames * channelIndex;
    // }

    // const Type* cend(int channelIndex) noexcept
    // {
    //     ASSERT(channelIndex >= 0);
    //     return buffer.data() + numFrames * (channelIndex + 1);
    // }

    Type& operator()(int channelIndex, int sampleIndex) noexcept
    {
        return getSample(channelIndex, sampleIndex);
    }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return NumChannels; }
    bool empty() const noexcept { return numFrames == 0; }

private:
    int numFrames { 0 };
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
        if (channelIndex < NumChannels)
            return &buffers[channelIndex].data();
        else
            return nullptr;
    }

    Type* begin(int channelIndex) noexcept
    {
        return buffers[channelIndex].begin();
    }

    Type* end(int channelIndex) noexcept
    {
        return buffers[channelIndex].end();
    }

    // const Type* cbegin(int channelIndex) noexcept
    // {
    //     return buffers[channelIndex].cbegin();
    // }

    // const Type* cend(int channelIndex) noexcept
    // {
    //     return buffers[channelIndex].cend();
    // }

    int getNumFrames() const noexcept { return numFrames; }
    int getNumChannels() const noexcept { return NumChannels; }
    bool empty() const noexcept { return numFrames == 0; }
private:
    int numFrames { 0 };
    std::array<Buffer<Type, Alignment>, NumChannels> buffers;
};