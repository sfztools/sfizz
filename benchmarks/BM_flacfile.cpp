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
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "ghc/filesystem.hpp"
#include <memory>
#ifndef NDEBUG
#include <iostream>
#endif
// #include "libnyquist/Decoders.h"

class FileFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state [[maybe_unused]]) {
        filePath1 = getPath() / "sample1.flac";
        filePath2 = getPath() / "sample2.flac";
        filePath3 = getPath() / "sample3.flac";
        if (    !ghc::filesystem::exists(filePath1) 
            ||  !ghc::filesystem::exists(filePath2)
            ||  !ghc::filesystem::exists(filePath3) ) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }
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

    std::unique_ptr<sfz::Buffer<float>> buffer;

    ghc::filesystem::path filePath1;
    ghc::filesystem::path filePath2;
    ghc::filesystem::path filePath3;
};


BENCHMARK_DEFINE_F(FileFixture, SndFile)(benchmark::State& state) {
    for (auto _ : state)
    {
        SndfileHandle sndfile(filePath1.c_str());
        buffer = std::make_unique<sfz::Buffer<float>>(sndfile.channels() * sndfile.frames());
        sndfile.readf(buffer->data(), sndfile.frames());
    }
}

BENCHMARK_DEFINE_F(FileFixture, DrFlac)(benchmark::State& state) {
    for (auto _ : state)
    {
        auto* flac = drflac_open_file(filePath2.c_str(), nullptr);
        buffer = std::make_unique<sfz::Buffer<float>>(flac->channels * flac->totalPCMFrameCount);
        drflac_read_pcm_frames_f32(flac, flac->totalPCMFrameCount, buffer->data());
    }
}

// BENCHMARK_DEFINE_F(FileFixture, LibNyquist)(benchmark::State& state) {
//     for (auto _ : state)
//     {
//         nqr::AudioData data;
//         nqr::NyquistIO loader;
//         loader.Load(&data, filePath3.string());
//         benchmark::DoNotOptimize(data);
//     }
// }

BENCHMARK_REGISTER_F(FileFixture, SndFile);
BENCHMARK_REGISTER_F(FileFixture, DrFlac);
// BENCHMARK_REGISTER_F(FileFixture, LibNyquist);
BENCHMARK_MAIN();
