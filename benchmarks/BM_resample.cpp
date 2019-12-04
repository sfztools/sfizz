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

#include "Buffer.h"
#include "Oversampler.h"
#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <memory>
#include <samplerate.h>
#include <sndfile.hh>
#include "ghc/filesystem.hpp"

class SndFile : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {

        const auto rootPath = getPath() / "ride.wav";
        if (!ghc::filesystem::exists(rootPath)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }

        SndfileHandle sndfile(rootPath.c_str());
        numFrames = sndfile.frames();
        numChannels = sndfile.channels();
        interleavedBuffer = std::make_unique<sfz::Buffer<float>>(numChannels * numFrames);
        sndfile.readf(interleavedBuffer->data(), sndfile.frames());
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]])
    {
    }

    ghc::filesystem::path getPath()
    {
        #ifdef __linux__
        char buf[PATH_MAX + 1];
        if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
            return {};
        std::string str { buf };
        return str.substr(0, str.rfind('/'));
        #elif _WIN32
        return ghc::filesystem::current_path();
        #endif
    }

    int numChannels;
    int numFrames;
    std::unique_ptr<sfz::Buffer<float>> interleavedBuffer;
};

BENCHMARK_DEFINE_F(SndFile, HIIR2X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample2x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR4X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample4x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR8X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample8x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR2X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample2x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR4X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample4x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR8X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved<float>(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = sfz::upsample8x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_FASTEST, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}


BENCHMARK_DEFINE_F(SndFile, SRC4x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC4x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC4x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_FASTEST, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = std::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = numFrames;
        srcData.output_frames = 2 * numFrames;
        src_simple(&srcData, SRC_SINC_FASTEST, numChannels);
        auto outBuffer = std::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved<float>(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}


BENCHMARK_REGISTER_F(SndFile, HIIR2X_scalar);
BENCHMARK_REGISTER_F(SndFile, HIIR4X_scalar);
BENCHMARK_REGISTER_F(SndFile, HIIR8X_scalar);
BENCHMARK_REGISTER_F(SndFile, HIIR2X_vector);
BENCHMARK_REGISTER_F(SndFile, HIIR4X_vector);
BENCHMARK_REGISTER_F(SndFile, HIIR8X_vector);
BENCHMARK_REGISTER_F(SndFile, SRC2x_BEST);
BENCHMARK_REGISTER_F(SndFile, SRC4x_BEST);
BENCHMARK_REGISTER_F(SndFile, SRC8x_BEST);
BENCHMARK_REGISTER_F(SndFile, SRC2x_MEDIUM);
BENCHMARK_REGISTER_F(SndFile, SRC4x_MEDIUM);
BENCHMARK_REGISTER_F(SndFile, SRC2x_FASTEST);
BENCHMARK_REGISTER_F(SndFile, SRC8x_MEDIUM);
BENCHMARK_REGISTER_F(SndFile, SRC4x_FASTEST);
BENCHMARK_REGISTER_F(SndFile, SRC8x_FASTEST);
BENCHMARK_MAIN();
