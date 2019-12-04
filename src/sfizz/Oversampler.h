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
#include "AudioBuffer.h"
#include "Config.h"
#include "hiir/Upsampler2xFpu.h"

namespace sfz {

constexpr std::array<double, 12> coeffsStage2x {
    0.036681502163648017,
    0.13654762463195771,
    0.27463175937945411,
    0.42313861743656667,
    0.56109869787919475,
    0.67754004997416162,
    0.76974183386322659,
    0.83988962484963803,
    0.89226081800387891,
    0.9315419599631839,
    0.96209454837808395,
    0.98781637073289708
};

constexpr std::array<double, 4> coeffsStage4x {
    0.042448989488488006,
    0.17072114107630679,
    0.39329183835224008,
    0.74569514831986694
};

constexpr std::array<double, 3> coeffsStage8x {
    0.055748680811302048,
    0.24305119574153092,
    0.6466991311926823
};

template<bool SIMD=SIMDConfig::upsampling>
void upsample2xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage2x.size()> upsampler;
    upsampler.set_coefs(coeffsStage2x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}


template<bool SIMD=SIMDConfig::upsampling>
void upsample4xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage4x.size()> upsampler;
    upsampler.set_coefs(coeffsStage4x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}

template<bool SIMD=SIMDConfig::upsampling>
void upsample8xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage8x.size()> upsampler;
    upsampler.set_coefs(coeffsStage8x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}

template<>
void upsample2xStage<true>(absl::Span<const float> input, absl::Span<float> output);
template<>
void upsample4xStage<true>(absl::Span<const float> input, absl::Span<float> output);
template<>
void upsample8xStage<true>(absl::Span<const float> input, absl::Span<float> output);

template <class T, bool SIMD=SIMDConfig::upsampling>
std::unique_ptr<sfz::AudioBuffer<T>> upsample2x(const sfz::AudioBuffer<T>& buffer)
{
    // auto tempBuffer = std::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto outputBuffer = std::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 2);
    for (int channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        sfz::upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}

template <class T, bool SIMD=SIMDConfig::upsampling>
std::unique_ptr<sfz::AudioBuffer<T>> upsample4x(const sfz::AudioBuffer<T>& buffer)
{
    auto tempBuffer = std::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto outputBuffer = std::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 4);
    for (int channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        sfz::upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), absl::MakeSpan(*tempBuffer));
        sfz::upsample4xStage<SIMD>(absl::MakeConstSpan(*tempBuffer), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}

template <class T, bool SIMD=SIMDConfig::upsampling>
std::unique_ptr<sfz::AudioBuffer<T>> upsample8x(const sfz::AudioBuffer<T>& buffer)
{
    auto tempBuffer2x = std::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto tempBuffer4x = std::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 4);
    auto outputBuffer = std::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 8);
    for (int channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        sfz::upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), absl::MakeSpan(*tempBuffer2x));
        sfz::upsample4xStage<SIMD>(absl::MakeConstSpan(*tempBuffer2x), absl::MakeSpan(*tempBuffer4x));
        sfz::upsample8xStage<SIMD>(absl::MakeConstSpan(*tempBuffer4x), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}

}
