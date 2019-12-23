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
#include <array>
#include <memory>
#include "absl/types/span.h"
#include "Debug.h"
#include "Buffer.h"
#include "AudioBuffer.h"
#include "Config.h"

namespace sfz {
/**
 * @brief Wraps the internal oversampler in a single function that takes an
 *        AudioBuffer and oversamples it in another pre-allocated one. The
 *        Oversampler processes the file in chunks and can signal the frames
 *        processes using an atomic counter.
 */
class Oversampler
{
public:
    /**
     * @brief Construct a new Oversampler object
     *
     * @param factor
     * @param chunkSize
     */
    Oversampler(Oversampling factor = Oversampling::x1, size_t chunkSize = config::chunkSize);
    /**
     * @brief Stream the oversampling of an input AudioBuffer into an output
     *        one, possibly signaling the caller along the way of the number of
     *        frames that are written.
     *
     * @param input
     * @param output
     * @param framesReady an atomic counter for the ready frames. If null no signaling is done.
     */
    void stream(const AudioBuffer<float>& input, AudioBuffer<float>& output, std::atomic<size_t>* framesReady = nullptr);

    Oversampler() = delete;
    Oversampler(const Oversampler&) = delete;
    Oversampler(Oversampler&&) = delete;
private:
    Oversampling factor;
    size_t chunkSize;

    LEAK_DETECTOR(Oversampler);
};

}
