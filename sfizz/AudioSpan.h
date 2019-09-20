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
#include "AudioBuffer.h"
#include "Buffer.h"
#include "Config.h"
#include "Debug.h"
#include "LeakDetector.h"
#include "SIMDHelpers.h"
#include <array>
#include <initializer_list>
#include <type_traits>

template <class Type, unsigned int MaxChannels = sfz::config::numChannels>
class AudioSpan {
public:
    using size_type = size_t;
    AudioSpan()
    {
    }

    AudioSpan(const std::array<Type*, MaxChannels>& spans, int numChannels, size_type offset, size_type size)
        : numFrames(size)
        , numChannels(numChannels)
    {
        ASSERT(static_cast<unsigned int>(numChannels) <= MaxChannels);
        for (auto i = 0; i < numChannels; ++i)
            this->spans[i] = spans[i] + offset;
    }

    AudioSpan(std::initializer_list<Type*> spans, size_type numFrames)
        : numFrames(numFrames)
        , numChannels(spans.size())
    {
        ASSERT(spans.size() <= MaxChannels);
        auto newSpan = spans.begin();
        auto thisSpan = this->spans.begin();
        for (; newSpan < spans.end() && thisSpan < this->spans.end(); thisSpan++, newSpan++) {
            // This will not end well...
            ASSERT(*newSpan != nullptr);
            *thisSpan = *newSpan;
        }
    }

    AudioSpan(std::initializer_list<absl::Span<Type>> spans)
        : numChannels(spans.size())
    {
        ASSERT(spans.size() <= MaxChannels);
        auto size = absl::Span<Type>::npos;
        auto newSpan = spans.begin();
        auto thisSpan = this->spans.begin();
        for (; newSpan < spans.end() && thisSpan < this->spans.end(); thisSpan++, newSpan++) {
            *thisSpan = newSpan->data();
            size = std::min(size, newSpan->size());
        }
    }

    template <class U, unsigned int N, unsigned int Alignment, typename = std::enable_if<N <= MaxChannels>, typename = std::enable_if_t<std::is_const<U>::value, int>>
    AudioSpan(AudioBuffer<U, N, Alignment>& audioBuffer)
        : numFrames(audioBuffer.getNumFrames())
        , numChannels(audioBuffer.getNumChannels())
    {
        for (int i = 0; i < numChannels; i++) {
            this->spans[i] = audioBuffer.channelReader(i);
        }
    }
    template <class U, unsigned int N, unsigned int Alignment, typename = std::enable_if<N <= MaxChannels>>
    AudioSpan(AudioBuffer<U, N, Alignment>& audioBuffer)
        : numFrames(audioBuffer.getNumFrames())
        , numChannels(audioBuffer.getNumChannels())
    {
        for (int i = 0; i < numChannels; i++) {
            this->spans[i] = audioBuffer.channelWriter(i);
        }
    }
    template <class U, unsigned int N, typename = std::enable_if<N <= MaxChannels>>
    AudioSpan(const AudioSpan<U, N>& other)
        : numFrames(other.getNumFrames())
        , numChannels(other.getNumChannels())
    {
        for (int i = 0; i < numChannels; i++) {
            this->spans[i] = other.getChannel(i);
        }
    }

    Type* getChannel(int channelIndex)
    {
        ASSERT(channelIndex < numChannels);
        if (channelIndex < numChannels)
            return spans[channelIndex];

        return {};
    }

    absl::Span<Type> getSpan(int channelIndex)
    {
        ASSERT(channelIndex < numChannels);
        if (channelIndex < numChannels)
            return { spans[channelIndex], numFrames };

        return {};
    }

    absl::Span<const Type> getConstSpan(int channelIndex)
    {
        ASSERT(channelIndex < numChannels);
        if (channelIndex < numChannels)
            return { spans[channelIndex], numFrames };

        return {};
    }

    Type meanSquared() noexcept
    {
        if (numChannels == 0)
            return 0.0;
        Type result = 0.0;
        for (int i = 0; i < numChannels; ++i)
            result += ::meanSquared<Type>(getConstSpan(i));
        return result / numChannels;
    }

    void fill(Type value) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            ::fill<Type>(getSpan(i), value);
    }

    void applyGain(absl::Span<const Type> gain) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            ::applyGain<Type>(gain, getSpan(i));
    }

    void applyGain(Type gain) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            ::applyGain<Type>(gain, getSpan(i));
    }

    template <class U, unsigned int N, typename = std::enable_if<N <= MaxChannels>>
    void add(AudioSpan<U, N>& other)
    {
        ASSERT(other.getNumChannels() == numChannels);
        if (other.getNumChannels() == numChannels) {
            for (int i = 0; i < numChannels; ++i)
                ::add<Type>(other.getConstSpan(i), getSpan(i));
        }
    }

    template <class U, unsigned int N, typename = std::enable_if<N <= MaxChannels>>
    void copy(AudioSpan<U, N>& other)
    {
        ASSERT(other.getNumChannels() == numChannels);
        if (other.getNumChannels() == numChannels) {
            for (int i = 0; i < numChannels; ++i)
                ::copy<Type>(other.getConstSpan(i), getSpan(i));
        }
    }

    size_type getNumFrames()
    {
        return numFrames;
    }

    int getNumChannels()
    {
        return numChannels;
    }

    AudioSpan<Type> first(size_type length)
    {
        ASSERT(length <= numFrames);
        return { spans, numChannels, 0, length };
    }

    AudioSpan<Type> last(size_type length)
    {
        ASSERT(length <= numFrames);
        return { spans, numChannels, numFrames - length, length };
    }

    AudioSpan<Type> subspan(size_type offset, size_type length)
    {
        ASSERT(length + offset <= numFrames);
        return { spans, numChannels, offset, length };
    }

    AudioSpan<Type> subspan(size_type offset)
    {
        ASSERT(offset <= numFrames);
        return { spans, numChannels, offset, numFrames - offset };
    }

private:
    std::array<Type*, MaxChannels> spans;
    size_type numFrames { 0 };
    int numChannels { 0 };
};