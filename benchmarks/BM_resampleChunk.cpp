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
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#include "ghc/filesystem.hpp"
#include "Buffer.h"
#include "SIMDHelpers.h"
#include "Oversampler.h"
#include "AudioBuffer.h"
#include <memory>
#include <iostream>

class FileFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /* state */) {
        rootPath = getPath() / "sample1.flac";
        if (!ghc::filesystem::exists(rootPath)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }

        sndfile = SndfileHandle(rootPath.c_str());
        numFrames = static_cast<size_t>(sndfile.frames());
        output = std::make_unique<sfz::AudioBuffer<float>>(sndfile.channels(), numFrames * 4);
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]]) {
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

    std::unique_ptr<sfz::AudioBuffer<float>> output;
    SndfileHandle sndfile;
    ghc::filesystem::path rootPath;
    size_t numFrames { 0 };
};

BENCHMARK_DEFINE_F(FileFixture, NoResampling)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Buffer<float> buffer { numFrames * sndfile.channels() };
        sndfile.readf(buffer.data(), sndfile.frames());
        sfz::readInterleaved<float>(buffer, output->getSpan(0), output->getSpan(1));
    }
}

BENCHMARK_DEFINE_F(FileFixture, ResampleAtOnce)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Buffer<float> buffer { numFrames * sndfile.channels() };
        sfz::Buffer<float> temp  { numFrames * 4 };
        sndfile.readf(buffer.data(), numFrames);
        sfz::readInterleaved<float>(buffer, output->getSpan(0), output->getSpan(1));

        sfz::upsample2xStage(output->getConstSpan(0).first(numFrames), absl::MakeSpan(temp));
        sfz::upsample4xStage(absl::MakeConstSpan(temp).first(numFrames * 2), output->getSpan(0));

        sfz::upsample2xStage(output->getConstSpan(1).first(numFrames), absl::MakeSpan(temp));
        sfz::upsample4xStage(absl::MakeConstSpan(temp).first(numFrames * 2), output->getSpan(1));
    }
}

BENCHMARK_DEFINE_F(FileFixture, ResampleInChunks)(benchmark::State& state) {
    for (auto _ : state)
    {
        auto chunkSize = static_cast<size_t>(state.range(0));
        sfz::Buffer<float> buffer { numFrames * sndfile.channels() };

        sfz::Buffer<float> leftInput  { chunkSize };
        sfz::Buffer<float> rightInput  { chunkSize };
        sfz::Buffer<float> chunk  { chunkSize * 2 };

        sndfile.readf(buffer.data(), numFrames);

        auto bufferSpan = absl::MakeSpan(buffer);
        auto leftSpan = absl::MakeSpan(leftInput);
        auto rightSpan = absl::MakeSpan(rightInput);
        auto chunkSpan = absl::MakeSpan(chunk);

        size_t inputFrameCounter { 0 };
        size_t outputFrameCounter { 0 };
        while(inputFrameCounter < numFrames)
        {
            // std::cout << "Input frames: " << inputFrameCounter << "/" << numFrames << '\n';
            const auto thisChunkSize = std::min(chunkSize, numFrames - inputFrameCounter);
            const auto bufferChunk = bufferSpan.subspan(
                inputFrameCounter * sndfile.channels(),
                thisChunkSize * sndfile.channels()
            );

            sfz::readInterleaved<float>(bufferChunk, leftSpan, rightSpan);
            sfz::upsample2xStage(leftSpan.first(thisChunkSize), chunkSpan);
            sfz::upsample4xStage(chunkSpan.first(thisChunkSize * 2), output->getSpan(0).subspan(outputFrameCounter));

            sfz::upsample2xStage(rightSpan.first(thisChunkSize), chunkSpan);
            sfz::upsample4xStage(chunkSpan.first(thisChunkSize * 2), output->getSpan(1).subspan(outputFrameCounter));

            inputFrameCounter += chunkSize;
            outputFrameCounter += chunkSize * 4;
        }
    }
}

BENCHMARK_REGISTER_F(FileFixture, NoResampling);
BENCHMARK_REGISTER_F(FileFixture, ResampleAtOnce);
BENCHMARK_REGISTER_F(FileFixture, ResampleInChunks)->RangeMultiplier(4)->Range((1 << 4), (1 << 16));
BENCHMARK_MAIN();
