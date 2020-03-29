// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Debug.h"
#include "Buffer.h"
#include "AudioBuffer.h"
#include <array>
#include <memory>
#ifndef NDEBUG
    #include "absl/algorithm/container.h"
    #include "MathHelpers.h"
#endif

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

        for (auto& buffer : indexBuffers) {
            buffer = std::make_shared<sfz::Buffer<int>>(config::defaultSamplesPerBlock);
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

        for (auto& buffer: indexBuffers) {
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

    std::shared_ptr<sfz::Buffer<float>> getBuffer(size_t numFrames) const
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

#ifndef NDEBUG
        maxBuffersUsed = max<int>(1 + absl::c_count_if(buffers, [&](const std::shared_ptr<sfz::Buffer<float>>& buffer) {
            return (buffer.use_count() > 1);
        }), maxBuffersUsed);
#endif

        while (bufferIt < buffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
            ++bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free buffer available!");
        return {};
    }

    std::shared_ptr<sfz::Buffer<int>> getIndexBuffer(size_t numFrames) const
    {
        auto bufferIt = indexBuffers.begin();

        if (indexBuffers.empty()) {
            DBG("[sfizz] No available index buffers in the pool");
            return {};
        }

        if (indexBuffers[0]->size() < numFrames) {
            DBG("[sfizz] Someone asked for a index buffer of size " << numFrames << "; only " << indexBuffers[0]->size() << " available...");
            return {};
        }

#ifndef NDEBUG
        maxIndexBuffersUsed = max<int>(1 + absl::c_count_if(indexBuffers, [&](const std::shared_ptr<sfz::Buffer<int>>& buffer) {
            return (buffer.use_count() > 1);
        }), maxIndexBuffersUsed);
#endif

        while (bufferIt < indexBuffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
            ++bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free index buffer available!");
        return {};
    }

    std::shared_ptr<sfz::AudioBuffer<float>> getStereoBuffer(size_t numFrames) const
    {
        if (stereoBuffers.empty()) {
            DBG("[sfizz] No available stereo buffers in the pool");
            return {};
        }

        if (stereoBuffers[0]->getNumFrames() < numFrames) {
            DBG("[sfizz] Someone asked for a stereo buffer of size " << numFrames << "; only " << stereoBuffers[0]->getNumFrames() << " available...");
            return {};
        }
#ifndef NDEBUG
        maxStereoBuffersUsed = max<int>(1 + absl::c_count_if(stereoBuffers, [&](const std::shared_ptr<sfz::AudioBuffer<float>>& buffer) {
            return (buffer.use_count() > 1);
        }), maxStereoBuffersUsed);
#endif
        auto bufferIt = stereoBuffers.begin();
        while (bufferIt < stereoBuffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
            ++bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free stereo buffer available!");
        return {};
    }

#ifndef NDEBUG
    ~BufferPool()
    {
        DBG("Max buffers used: " << maxBuffersUsed);
        DBG("Max index buffers used: " << maxIndexBuffersUsed);
        DBG("Max stereo buffers used: " << maxStereoBuffersUsed);
    }
#endif
private:
    std::array<std::shared_ptr<sfz::Buffer<float>>, config::bufferPoolSize> buffers;
    std::array<std::shared_ptr<sfz::Buffer<int>>, config::bufferPoolSize> indexBuffers;
    std::array<std::shared_ptr<sfz::AudioBuffer<float>>, config::stereoBufferPoolSize> stereoBuffers;
#ifndef NDEBUG
    mutable int maxBuffersUsed { 0 };
    mutable int maxIndexBuffersUsed { 0 };
    mutable int maxStereoBuffersUsed { 0 };
#endif
};
}
