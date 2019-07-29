#include "catch2/catch.hpp"
#include "../sources/AudioBuffer.h"
#include <algorithm>
using namespace Catch::literals;

TEST_CASE("[AudioBuffer/SplitBuffer] Empty buffers")
{
    AudioBuffer<float> floatBuffer;
    REQUIRE(floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 0);
    AudioBuffer<double> doubleBuffer;
    REQUIRE(doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 0);
    AudioBuffer<int> intBuffer;
    REQUIRE(intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 0);

    SplitAudioBuffer<float> floatSplitBuffer;
    REQUIRE(floatSplitBuffer.empty());
    REQUIRE(floatSplitBuffer.getNumFrames() == 0);
    SplitAudioBuffer<double> doubleSplitBuffer;
    REQUIRE(doubleSplitBuffer.empty());
    REQUIRE(doubleSplitBuffer.getNumFrames() == 0);
    SplitAudioBuffer<int> intSplitBuffer;
    REQUIRE(intSplitBuffer.empty());
    REQUIRE(intSplitBuffer.getNumFrames() == 0);
}

TEST_CASE("[AudioBuffer/SplitBuffer] Non-empty")
{
    AudioBuffer<float> floatBuffer(10);
    REQUIRE(!floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 10);
    AudioBuffer<double> doubleBuffer(10);
    REQUIRE(!doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 10);
    AudioBuffer<int> intBuffer(10);
    REQUIRE(!intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 10);

    SplitAudioBuffer<float> floatSplitBuffer(10);
    REQUIRE(!floatSplitBuffer.empty());
    REQUIRE(floatSplitBuffer.getNumFrames() == 10);
    SplitAudioBuffer<double> doubleSplitBuffer(10);
    REQUIRE(!doubleSplitBuffer.empty());
    REQUIRE(doubleSplitBuffer.getNumFrames() == 10);
    SplitAudioBuffer<int> intSplitBuffer(10);
    REQUIRE(!intSplitBuffer.empty());
    REQUIRE(intSplitBuffer.getNumFrames() == 10);
}

TEST_CASE("[AudioBuffer/SplitBuffer] Access")
{
    const int size { 5 };
    AudioBuffer<double> doubleBuffer(size);
    for (auto chanIdx = 0; chanIdx < doubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
            doubleBuffer.getSample(chanIdx, frameIdx) = static_cast<double>(doubleBuffer.getNumFrames()) * chanIdx + frameIdx;

    for (auto chanIdx = 0; chanIdx < doubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
            REQUIRE(doubleBuffer.getSample(chanIdx, frameIdx) == static_cast<double>(doubleBuffer.getNumFrames()) * chanIdx + frameIdx);

    SplitAudioBuffer<double> splitDoubleBuffer(size);
    for (auto chanIdx = 0; chanIdx < splitDoubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < splitDoubleBuffer.getNumFrames(); ++frameIdx)
            splitDoubleBuffer.getSample(chanIdx, frameIdx) = static_cast<double>(splitDoubleBuffer.getNumFrames()) * chanIdx + frameIdx;

    for (auto chanIdx = 0; chanIdx < splitDoubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < splitDoubleBuffer.getNumFrames(); ++frameIdx)
            REQUIRE(splitDoubleBuffer.getSample(chanIdx, frameIdx) == static_cast<double>(splitDoubleBuffer.getNumFrames()) * chanIdx + frameIdx);
}

TEST_CASE("[AudioBuffer/SplitBuffer] Access 2")
{
    const int size { 5 };
    AudioBuffer<int> doubleBuffer(size);
    for (auto chanIdx = 0; chanIdx < doubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
            doubleBuffer(chanIdx, frameIdx) = static_cast<int>(doubleBuffer.getNumFrames()) * chanIdx + frameIdx;

    for (auto chanIdx = 0; chanIdx < doubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
            REQUIRE(doubleBuffer(chanIdx, frameIdx) == static_cast<int>(doubleBuffer.getNumFrames()) * chanIdx + frameIdx);

    SplitAudioBuffer<int> splitDoubleBuffer(size);
    for (auto chanIdx = 0; chanIdx < splitDoubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < splitDoubleBuffer.getNumFrames(); ++frameIdx)
            splitDoubleBuffer(chanIdx, frameIdx) = static_cast<int>(splitDoubleBuffer.getNumFrames()) * chanIdx + frameIdx;

    for (auto chanIdx = 0; chanIdx < splitDoubleBuffer.getNumChannels(); ++chanIdx)
        for (auto frameIdx = 0; frameIdx < splitDoubleBuffer.getNumFrames(); ++frameIdx)
            REQUIRE(splitDoubleBuffer(chanIdx, frameIdx) == static_cast<int>(splitDoubleBuffer.getNumFrames()) * chanIdx + frameIdx);
}