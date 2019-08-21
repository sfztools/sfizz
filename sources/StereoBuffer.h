#pragma once
#include "Buffer.h"
#include "Helpers.h"
#include "Globals.h"

#include <array>
#include <iostream>
#include <type_traits>
#include "SIMDHelpers.h"

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

    void fill(Type value) noexcept
    {
        ::fill<Type>(absl::MakeSpan(leftBuffer), value);
        ::fill<Type>(absl::MakeSpan(rightBuffer), value);
    }

    void readInterleaved(absl::Span<const Type> input) noexcept
    {
        ASSERT(input.size() <= numChannels * numFrames);
        ::readInterleaved<Type>(input, absl::MakeSpan(leftBuffer), absl::MakeSpan(rightBuffer));
    }

    void writeInterleaved(absl::Span<Type> output) noexcept
    {
        ASSERT(output.size() >= numChannels * numFrames);
        ::writeInterleaved<Type>(leftBuffer, rightBuffer, output);
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
