// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/AudioBuffer.h"
#include "sfizz/AudioSpan.h"
#include "catch2/catch.hpp"
#include <algorithm>
using namespace Catch::literals;

TEST_CASE("[AudioBuffer] Empty buffers")
{
    sfz::AudioBuffer<float> floatBuffer;
    REQUIRE(floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 0);
    sfz::AudioBuffer<double> doubleBuffer;
    REQUIRE(doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 0);
    sfz::AudioBuffer<int> intBuffer;
    REQUIRE(intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 0);
}

TEST_CASE("[AudioBuffer] Non-empty")
{
    sfz::AudioBuffer<float> floatBuffer(1, 10);
    REQUIRE(!floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 10);
    REQUIRE(floatBuffer.getNumChannels() == 1);
    sfz::AudioBuffer<double> doubleBuffer(2, 10);
    REQUIRE(!doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 10);
    REQUIRE(doubleBuffer.getNumChannels() == 2);
    sfz::AudioBuffer<int> intBuffer(1, 10);
    REQUIRE(!intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 10);
    REQUIRE(intBuffer.getNumChannels() == 1);
}

TEST_CASE("[AudioBuffer] Access")
{
    const int size { 5 };
    sfz::AudioBuffer<float> buffer(2, size);
    for (size_t frameIdx = 0; frameIdx < buffer.getNumFrames(); ++frameIdx) {
        buffer.getSample(0, frameIdx) = static_cast<float>(buffer.getNumFrames()) + frameIdx;
        buffer.getSample(1, frameIdx) = static_cast<float>(buffer.getNumFrames()) - frameIdx;
    }

    for (size_t frameIdx = 0; frameIdx < buffer.getNumFrames(); ++frameIdx) {
        REQUIRE(buffer.getSample(0, frameIdx) == static_cast<float>(buffer.getNumFrames()) + frameIdx);
        REQUIRE(buffer(0, frameIdx) == static_cast<float>(buffer.getNumFrames()) + frameIdx);
        REQUIRE(buffer.getSample(1, frameIdx) == static_cast<float>(buffer.getNumFrames()) - frameIdx);
        REQUIRE(buffer(1, frameIdx) == static_cast<float>(buffer.getNumFrames()) - frameIdx);
    }
}

TEST_CASE("[AudioBuffer] Iterators")
{
    const int size { 256 };
    const float fillValue { 2.0f };
    sfz::AudioBuffer<float> buffer(2, size);
    std::fill(buffer.channelWriter(0), buffer.channelWriterEnd(0), fillValue);
    std::fill(buffer.channelWriter(1), buffer.channelWriterEnd(1), fillValue);

    REQUIRE(std::all_of(buffer.channelReader(0), buffer.channelReaderEnd(0), [fillValue](float value) { return value == fillValue; }));
    REQUIRE(std::all_of(buffer.channelReader(1), buffer.channelReaderEnd(1), [fillValue](float value) { return value == fillValue; }));
}

TEST_CASE("[AudioSpan] Constructions")
{
    const int size { 256 };
    const float fillValue { 2.0f };
    sfz::AudioBuffer<float> buffer(2, size);
    std::fill(buffer.channelWriter(0), buffer.channelWriterEnd(0), fillValue);
    std::fill(buffer.channelWriter(1), buffer.channelWriterEnd(1), fillValue);

    sfz::AudioSpan<float> span { buffer };
    sfz::AudioSpan<const float> constSpan { buffer };
    sfz::AudioSpan<float> manualSpan { { buffer.channelWriter(0), buffer.channelWriter(1) }, buffer.getNumFrames() };
    sfz::AudioSpan<const float> manualConstSpan { { buffer.channelReader(0), buffer.channelReader(1) }, buffer.getNumFrames() };
    sfz::AudioSpan<float> manualSpan2 { {buffer.getSpan(0), buffer.getSpan(1) } };
    sfz::AudioSpan<const float> manualConstSpan2 { { buffer.getConstSpan(0), buffer.getConstSpan(1) } };
}
