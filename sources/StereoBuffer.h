// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "Buffer.h"
#include "Globals.h"
#include "Debug.h"
#include "LeakDetector.h"
#include "SIMDHelpers.h"
#include <array>
#include <iostream>
#include <type_traits>

enum class Channel { left,
    right };

template <class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class StereoBuffer {
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
        if (leftBuffer.resize(static_cast<size_t>(numFrames)) && rightBuffer.resize(static_cast<size_t>(numFrames))) {
            this->numFrames = numFrames;
            return true;
        }

        return false;
    }

    absl::Span<const Type> getConstSpan(Channel channel) const
    {
        switch (channel) {
        case Channel::left:
            return leftBuffer;
        case Channel::right:
            return rightBuffer;
        }
    }

    absl::Span<Type> getSpan(Channel channel)
    {
        switch (channel) {
        default:
            [[fallthrough]];
        case Channel::left:
            return absl::MakeSpan(leftBuffer);
        case Channel::right:
            return absl::MakeSpan(rightBuffer);
        }
    }

    Type& getSample(Channel channel, int sampleIndex) noexcept
    {
        ASSERT(sampleIndex >= 0);
        switch (channel) {
        default:
            [[fallthrough]];
        case Channel::left:
            return leftBuffer[sampleIndex];
        case Channel::right:
            return rightBuffer[sampleIndex];
        }
    }

    void fill(Type value) noexcept
    {
        ::fill<Type>(absl::MakeSpan(leftBuffer), value);
        ::fill<Type>(absl::MakeSpan(rightBuffer), value);
    }

    void readInterleaved(absl::Span<const Type> input) noexcept
    {
        ASSERT(input.size() <= static_cast<size_t>(numChannels * numFrames));
        ::readInterleaved<Type>(input, absl::MakeSpan(leftBuffer), absl::MakeSpan(rightBuffer));
    }

    void writeInterleaved(absl::Span<Type> output) noexcept
    {
        ASSERT(output.size() >= static_cast<size_t>(numChannels * numFrames));
        ::writeInterleaved<Type>(leftBuffer, rightBuffer, output);
    }

    void add(const StereoBuffer<Type>& buffer)
    {
        ::add<Type>(buffer.getSpan(Channel::left), absl::MakeSpan(leftBuffer));
        ::add<Type>(buffer.getSpan(Channel::right), absl::MakeSpan(rightBuffer));
    }

    Type* getChannel(Channel channel) noexcept
    {
        switch (channel) {
        case Channel::left:
            return leftBuffer.data();
        case Channel::right:
            return rightBuffer.data();
        default:
            return {};
        }
    }

    Type* begin(Channel channel) noexcept
    {
        switch (channel) {
        case Channel::left:
            return leftBuffer.data();
        case Channel::right:
            return rightBuffer.data();
        default:
            return {};
        }
    }
    std::pair<Type*, Type*> getChannels() noexcept { return { leftBuffer.data(), rightBuffer.data() }; }
    std::pair<Type*, Type*> begins() noexcept { return { leftBuffer.data(), rightBuffer.data() }; }

    Type* end(Channel channel) noexcept
    {
        switch (channel) {
        case Channel::left:
            return leftBuffer.end();
        case Channel::right:
            return rightBuffer.end();
        default:
            return {};
        }
    }
    std::pair<Type*, Type*> ends() { return { leftBuffer.end(), rightBuffer.end() }; }

    Type* alignedEnd(Channel channel) noexcept
    {
        switch (channel) {
        case Channel::left:
            return leftBuffer.alignedEnd();
        case Channel::right:
            return rightBuffer.alignedEnd();
        default:
            return {};
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
    Buffer<Type, Alignment> leftBuffer {};
    Buffer<Type, Alignment> rightBuffer {};
    LEAK_DETECTOR(StereoBuffer);
};
