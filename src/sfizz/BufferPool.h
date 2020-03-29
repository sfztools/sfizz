// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Buffer.h"
#include <array>
#include <memory>

namespace sfz
{

class BufferPool
{
public:
    BufferPool()
    {

    }
    void setBufferSize(unsigned bufferSize)
    {
        for (auto& buffer: buffers) {
            // Trying to resize a buffer in use
            ASSERT(buffer.use_count() != 1);
            buffer->resize(bufferSize);
        }
    }

    std::shared_ptr<sfz::Buffer<float>> getBuffer()
    {
        auto bufferIt = buffers.begin();
        while (bufferIt < buffers.end()) {
            if (bufferIt->use_count() == 1)
                return *bufferIt;
        }

        // No buffer found; debug message
        DBG("[sfizz] No free buffer available!");
    }
private:
    std::array<std::shared_ptr<sfz::Buffer<float>>, config::bufferPoolSize> buffers;
};
}
