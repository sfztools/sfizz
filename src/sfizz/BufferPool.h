// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Buffer.h"
#include "AudioBuffer.h"
#include <array>
#include <memory>

namespace sfz
{

class BufferPool
{
public:
    BufferPool()
    {
        for (auto& buffer : buffers) {
            buffer = std::make_shared<sfz::Buffer<float>>(config::defaultSamplesPerBlock);
        }

        for (auto& buffer : stereoBuffers) {
            buffer = std::make_shared<sfz::AudioBuffer<float>>(2, config::defaultSamplesPerBlock);
        }
    }

    void setBufferSize(unsigned bufferSize)
    {
        for (auto& buffer: buffers) {
            // Trying to resize a buffer in use
            ASSERT(buffer.use_count() == 1);
            buffer->resize(bufferSize);
        }

        for (auto& buffer: stereoBuffers) {
            // Trying to resize a buffer in use
            ASSERT(buffer.use_count() == 1);
            buffer->resize(bufferSize);
        }
    }

    std::shared_ptr<sfz::Buffer<float>> getBuffer(size_t numFrames)
    {
        auto bufferIt = buffers.begin();

        if (buffers.empty()) {
            DBG("[sfizz] No available buffers in the pool");
            return {};
        }

        if (buffers[0]->size() < numFrames) {
            DBG("[sfizz] Someone asked for a buffer of size " << numFrames << "; only " << buffers[0]->size() << " available...");
            return {};
        }

        while (bufferIt < buffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
            ++bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free buffer available!");
        return {};
    }

    std::shared_ptr<sfz::AudioBuffer<float>> getStereoBuffer(size_t numFrames)
    {
        if (stereoBuffers.empty()) {
            DBG("[sfizz] No available stereoBuffers in the pool");
            return {};
        }

        if (stereoBuffers[0]->getNumFrames() < numFrames) {
            DBG("[sfizz] Someone asked for a buffer of size " << numFrames << "; only " << stereoBuffers[0]->getNumFrames() << " available...");
            return {};
        }

        auto bufferIt = stereoBuffers.begin();
        while (bufferIt < stereoBuffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
            ++bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free buffer available!");
        return {};
    }
private:
    std::array<std::shared_ptr<sfz::Buffer<float>>, config::bufferPoolSize> buffers;
    std::array<std::shared_ptr<sfz::AudioBuffer<float>>, config::stereoBufferPoolSize> stereoBuffers;
};
}
