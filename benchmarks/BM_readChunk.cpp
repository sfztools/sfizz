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
#include "SIMDHelpers.h"
#include "Buffer.h"
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#include "ghc/filesystem.hpp"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "AudioBuffer.h"
#include <memory>
#ifndef NDEBUG
#include <iostream>
#endif

class FileFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /* state */) {
        rootPath = getPath() / "sample1.wav";
        if (!ghc::filesystem::exists(rootPath)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }

        sndfile = SndfileHandle(rootPath.c_str());
        numFrames = static_cast<size_t>(sndfile.frames());
        output = std::make_unique<sfz::AudioBuffer<float>>(sndfile.channels(), sndfile.frames());
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
    size_t numFrames;
};


BENCHMARK_DEFINE_F(FileFixture, JustRead)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Buffer<float> buffer { numFrames * sndfile.channels() };
        sndfile.readf(buffer.data(), numFrames);
        sfz::readInterleaved<float>(buffer, output->getSpan(0), output->getSpan(1));
    }
}

BENCHMARK_DEFINE_F(FileFixture, AllocInside)(benchmark::State& state) {
    for (auto _ : state)
    {
        auto chunkSize = static_cast<size_t>(state.range(0));
        size_t framesRead = 0;
        sndfile.seek(0, SEEK_SET);
        while(framesRead < numFrames)
        {
            sfz::Buffer<float> buffer { chunkSize * sndfile.channels() };
            auto read = sndfile.readf(buffer.data(), chunkSize);
            sfz::readInterleaved<float>(
                absl::MakeSpan(buffer).first(read),
                output->getSpan(0).subspan(framesRead),
                output->getSpan(1).subspan(framesRead)
            );
            framesRead += read;
        }
    }
}

BENCHMARK_DEFINE_F(FileFixture, AllocOutside)(benchmark::State& state) {
    auto chunkSize = static_cast<size_t>(state.range(0));
    sfz::Buffer<float> buffer { chunkSize * sndfile.channels() };
    for (auto _ : state)
    {
        size_t framesRead = 0;
        sndfile.seek(0, SEEK_SET);
        while(framesRead < numFrames)
        {
            auto read = sndfile.readf(buffer.data(), chunkSize);
            sfz::readInterleaved<float>(
                absl::MakeSpan(buffer).first(read),
                output->getSpan(0).subspan(framesRead),
                output->getSpan(1).subspan(framesRead)
            );
            framesRead += read;
        }
    }
}


BENCHMARK_DEFINE_F(FileFixture, DrWavChunked)(benchmark::State& state) {
    auto chunkSize = static_cast<size_t>(state.range(0));
    drwav wav;
    if (!drwav_init_file(&wav, rootPath.c_str(), nullptr)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
        std::terminate();
    }
    sfz::Buffer<float> buffer { chunkSize * wav.channels };

    for (auto _ : state)
    {
        size_t framesRead = 0;
        drwav_seek_to_first_pcm_frame(&wav);
        while(framesRead < numFrames)
        {
            auto read = drwav_read_pcm_frames_f32(&wav, chunkSize, buffer.data());
            sfz::readInterleaved<float>(
                absl::MakeSpan(buffer).first(read),
                output->getSpan(0).subspan(framesRead),
                output->getSpan(1).subspan(framesRead)
            );
            framesRead += read;
        }
    }
}

BENCHMARK_REGISTER_F(FileFixture, JustRead);
BENCHMARK_REGISTER_F(FileFixture, AllocInside)->RangeMultiplier(4)->Range((1 << 8), (1 << 16));
BENCHMARK_REGISTER_F(FileFixture, AllocOutside)->RangeMultiplier(4)->Range((1 << 8), (1 << 16));
BENCHMARK_REGISTER_F(FileFixture, DrWavChunked)->RangeMultiplier(4)->Range((1 << 8), (1 << 16));
BENCHMARK_MAIN();
