#include "catch2/catch.hpp"
#include "../sources/StereoBuffer.h"
#include <algorithm>
using namespace Catch::literals;

TEST_CASE("[StereoBuffer] Empty buffers")
{
    StereoBuffer<float> floatBuffer;
    REQUIRE(floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 0);
    StereoBuffer<double> doubleBuffer;
    REQUIRE(doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 0);
    StereoBuffer<int> intBuffer;
    REQUIRE(intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 0);
}

TEST_CASE("[StereoBuffer] Non-empty")
{
    StereoBuffer<float> floatBuffer(10);
    REQUIRE(!floatBuffer.empty());
    REQUIRE(floatBuffer.getNumFrames() == 10);
    StereoBuffer<double> doubleBuffer(10);
    REQUIRE(!doubleBuffer.empty());
    REQUIRE(doubleBuffer.getNumFrames() == 10);
    StereoBuffer<int> intBuffer(10);
    REQUIRE(!intBuffer.empty());
    REQUIRE(intBuffer.getNumFrames() == 10);
}

TEST_CASE("[StereoBuffer] Access")
{
    const int size { 5 };
    StereoBuffer<double> doubleBuffer(size);
    for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
    {
        doubleBuffer.getSample(Channel::left, frameIdx) = static_cast<double>(doubleBuffer.getNumFrames()) + frameIdx;
        doubleBuffer.getSample(Channel::right, frameIdx) = static_cast<double>(doubleBuffer.getNumFrames()) - frameIdx;
    }

    for (auto frameIdx = 0; frameIdx < doubleBuffer.getNumFrames(); ++frameIdx)
    {
        REQUIRE(doubleBuffer.getSample(Channel::left, frameIdx) == static_cast<double>(doubleBuffer.getNumFrames())  + frameIdx);
        REQUIRE(doubleBuffer(Channel::left, frameIdx) == static_cast<double>(doubleBuffer.getNumFrames())  + frameIdx);
        REQUIRE(doubleBuffer.getSample(Channel::right, frameIdx) == static_cast<double>(doubleBuffer.getNumFrames()) - frameIdx);
        REQUIRE(doubleBuffer(Channel::right, frameIdx) == static_cast<double>(doubleBuffer.getNumFrames()) - frameIdx);
    }
}

TEST_CASE("[StereoBuffer] Iterators")
{
    const int size { 256 };
    const float fillValue { 2.0f };
    StereoBuffer<float> buffer(size);
    std::fill(buffer.begin(Channel::left), buffer.end(Channel::left), fillValue);
    std::fill(buffer.begin(Channel::right), buffer.end(Channel::right), fillValue);

    REQUIRE( std::all_of(buffer.begin(Channel::left), buffer.end(Channel::left), [fillValue](auto value) { return value == fillValue; }) );
    REQUIRE( std::all_of(buffer.begin(Channel::right), buffer.end(Channel::right), [fillValue](auto value) { return value == fillValue; }) );
}

template<class Type, unsigned int Alignment = 16>
void channelAlignmentTest(int size)
{
    static constexpr auto AlignmentMask { Alignment - 1 };
    StereoBuffer<Type, Alignment> buffer(size);
    REQUIRE( ((size_t)buffer.getChannel(Channel::left) & AlignmentMask) == 0 );
    REQUIRE( ((size_t)buffer.getChannel(Channel::right) & AlignmentMask) == 0 );
}

TEST_CASE("[StereoBuffer] Channel alignments (floats)")
{
    channelAlignmentTest<float>(4);
    channelAlignmentTest<float>(5);
    channelAlignmentTest<float>(8);
    channelAlignmentTest<float>(256);
    channelAlignmentTest<float>(257);
    channelAlignmentTest<float>(1023);
    channelAlignmentTest<float>(1024);
    channelAlignmentTest<float>(65537);
    channelAlignmentTest<float>(65536);
    channelAlignmentTest<float>(65535);

    channelAlignmentTest<float, 4>(4);
    channelAlignmentTest<float, 4>(5);
    channelAlignmentTest<float, 4>(8);
    channelAlignmentTest<float, 4>(256);
    channelAlignmentTest<float, 4>(257);
    channelAlignmentTest<float, 4>(1023);
    channelAlignmentTest<float, 4>(1024);
    channelAlignmentTest<float, 4>(65537);
    channelAlignmentTest<float, 4>(65536);
    channelAlignmentTest<float, 4>(65535);

    channelAlignmentTest<float, 8>(4);
    channelAlignmentTest<float, 8>(5);
    channelAlignmentTest<float, 8>(8);
    channelAlignmentTest<float, 8>(256);
    channelAlignmentTest<float, 8>(257);
    channelAlignmentTest<float, 8>(1023);
    channelAlignmentTest<float, 8>(1024);
    channelAlignmentTest<float, 8>(65537);
    channelAlignmentTest<float, 8>(65536);
    channelAlignmentTest<float, 8>(65535);
}

TEST_CASE("[StereoBuffer] Channel alignments (doubles)")
{
    channelAlignmentTest<double>(4);
    channelAlignmentTest<double>(5);
    channelAlignmentTest<double>(8);
    channelAlignmentTest<double>(256);
    channelAlignmentTest<double>(257);
    channelAlignmentTest<double>(1023);
    channelAlignmentTest<double>(1024);
    channelAlignmentTest<double>(65537);
    channelAlignmentTest<double>(65536);
    channelAlignmentTest<double>(65535);

    channelAlignmentTest<double, 8>(4);
    channelAlignmentTest<double, 8>(5);
    channelAlignmentTest<double, 8>(8);
    channelAlignmentTest<double, 8>(256);
    channelAlignmentTest<double, 8>(257);
    channelAlignmentTest<double, 8>(1023);
    channelAlignmentTest<double, 8>(1024);
    channelAlignmentTest<double, 8>(65537);
    channelAlignmentTest<double, 8>(65536);
    channelAlignmentTest<double, 8>(65535);
}

TEST_CASE("[AudioBuffer] fills")
{
    StereoBuffer<float> buffer(10);
    buffer.fill(1.3f);
    std::array<float, 10> expected;
    std::fill(expected.begin(), expected.end(), 1.3f);
    std::array<float, 10> real { 0.0f };

    for (auto frameIdx = 0; frameIdx < buffer.getNumFrames(); ++frameIdx)
        real[frameIdx] = buffer(Channel::left, frameIdx);
    REQUIRE( real == expected );

    for (auto frameIdx = 0; frameIdx < buffer.getNumFrames(); ++frameIdx)
        real[frameIdx] = buffer(Channel::right, frameIdx);
    REQUIRE( real == expected );
}


TEST_CASE("[AudioBuffer] Fill a big Audiobuffer")
{
    constexpr int size { 2039247 };
    StereoBuffer<float> buffer (size);
    std::vector<float> input (2*size);
    std::iota(input.begin(), input.end(), 1.0f);
    buffer.readInterleaved(input);
}

TEST_CASE("[StereoBuffer] Interleaved write -- Scalar")
{
    StereoBuffer<float> buffer(10);
    std::array<float, 20> input  = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f};
    std::array<float, 20> output { 0.0f };
    buffer.readInterleaved(input);
    buffer.writeInterleaved(absl::MakeSpan(output));
    REQUIRE( output == input );
}

TEST_CASE("[StereoBuffer] Interleaved write -- SIMD")
{
    StereoBuffer<float> buffer(10);
    std::array<float, 20> input  = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f, 6.0f, 16.0f, 7.0f, 17.0f, 8.0f, 18.0f, 9.0f, 19.0f};
    std::array<float, 20> output { 0.0f };
    buffer.readInterleaved(input);
    buffer.writeInterleaved(absl::MakeSpan(output));
    REQUIRE( output == input );
}

TEST_CASE("[StereoBuffer] Small interleaved write -- SIMD")
{
    StereoBuffer<float> buffer(3);
    std::array<float, 6> input  = { 0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f};
    std::array<float, 6> output { 0.0f };
    buffer.readInterleaved(input);
    buffer.writeInterleaved(absl::MakeSpan(output));
    REQUIRE( output == input );
}