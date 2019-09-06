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
#include "Config.h"
#include "Debug.h"
#include "LeakDetector.h"
#include "absl/types/span.h"
#include <memory>

template <class Type, unsigned int MaxChannels = sfz::config::numChannels, unsigned int Alignment = SIMDConfig::defaultAlignment>
class AudioBuffer {
public:
    using value_type = std::remove_cv_t<Type>;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using size_type = size_t;

    AudioBuffer()
    {
    }
    AudioBuffer(int numChannels, int numFrames)
        : numChannels(numChannels)
        , numFrames(numFrames)
    {
        for (auto i = 0; i < numChannels; ++i)
            buffers[i] = std::make_unique<buffer_type>(numFrames);
    }

    iterator channelWriter(int channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->data();

        return {};
    }

    iterator channelWriterEnd(int channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->end();

        return {};
    }

    const_iterator channelReader(int channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->data();

        return {};
    }

    const_iterator channelReaderEnd(int channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return buffers[channelIndex]->end();

        return {};
    }

    absl::Span<value_type> getSpan(int channelIndex)
    {
        ASSERT(channelIndex < numChannels)
        if (channelIndex < numChannels)
            return { buffers[channelIndex]->data(), buffers[channelIndex]->size() };

        return {};
    }

    absl::Span<const value_type> getConstSpan(int channelIndex)
    {
        return getSpan(channelIndex);
    }

   	void addChannel()
   	{
   		if (numChannels < MaxChannels)
   			buffers[numChannels++] = std::make_unique<buffer_type>(numFrames);
   	}

   	size_type getNumFrames()
   	{
   		return numFrames;
   	}

	size_type getNumChannels()
   	{
   		return numChannels;
   	}

   	bool empty()
   	{
   		return numFrames == 0;
   	}

   	Type& getSample(int channelIndex, size_type frameIndex)
   	{
   		// Uhoh
   		ASSERT(buffers[channelIndex] != nullptr);
   		ASSERT(frameIndex < numFrames);

   		return *(buffers[channelIndex]->data() + frameIndex);
   	}

   	Type& operator()(int channelIndex, size_type frameIndex)
   	{
   		return getSample(channelIndex, frameIndex);
   	}

private:
    using buffer_type = Buffer<Type, Alignment>;
    using buffer_ptr = std::unique_ptr<buffer_type>;
    std::array<buffer_ptr, MaxChannels> buffers;
    int numChannels { 0 };
    size_type numFrames { 0 };
};