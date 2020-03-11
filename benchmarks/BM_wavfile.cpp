// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz
#include "Buffer.h"
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "ghc/filesystem.hpp"
#include "absl/memory/memory.h"
#ifndef NDEBUG
#include <iostream>
#endif
// #include "libnyquist/Decoders.h"

class FileFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /* state */) {
        filePath1 = getPath() / "sample1.wav";
        filePath2 = getPath() / "sample2.wav";
        filePath3 = getPath() / "sample3.wav";
        if (   !ghc::filesystem::exists(filePath1)
            || !ghc::filesystem::exists(filePath2)
            || !ghc::filesystem::exists(filePath3)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }
    }

    void TearDown(const ::benchmark::State& /* state */) {
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
        buffer = absl::make_unique<sfz::Buffer<float>>(sndfile.channels() * sndfile.frames());
        sndfile.readf(buffer->data(), sndfile.frames());
    }
}

BENCHMARK_DEFINE_F(FileFixture, DrWav)(benchmark::State& state) {
    for (auto _ : state)
    {
        drwav wav;
        if (!drwav_init_file(&wav, filePath2.c_str(), nullptr)) {
            #ifndef NDEBUG
                std::cerr << "Can't find path" << '\n';
            #endif
            std::terminate();
        }
        buffer = absl::make_unique<sfz::Buffer<float>>(wav.channels * wav.totalPCMFrameCount);
        drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, buffer->data());
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
BENCHMARK_REGISTER_F(FileFixture, DrWav);
// BENCHMARK_REGISTER_F(FileFixture, LibNyquist);
BENCHMARK_MAIN();
