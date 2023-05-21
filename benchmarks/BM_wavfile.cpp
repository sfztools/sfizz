// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Buffer.h"
#include <benchmark/benchmark.h>
#if defined(SFIZZ_USE_SNDFILE)
#include <sndfile.hh>
#endif
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "ghc/fs_std.hpp"
#include "absl/memory/memory.h"
#if 0
#include "libnyquist/Decoders.h"
#endif
#ifndef NDEBUG
#include <iostream>
#endif
#include <unistd.h> // readlink

class FileFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /* state */) {
        filePath1 = getPath() / "sample1.wav";
        filePath2 = getPath() / "sample2.wav";
        filePath3 = getPath() / "sample3.wav";
        if (   !fs::exists(filePath1)
            || !fs::exists(filePath2)
            || !fs::exists(filePath3)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }
    }

    void TearDown(const ::benchmark::State& /* state */) {
    }

    fs::path getPath()
    {
        #ifdef __linux__
        char buf[PATH_MAX + 1];
        if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
            return {};
        std::string str { buf };
        return str.substr(0, str.rfind('/'));
        #elif _WIN32
        return fs::current_path();
        #endif
    }

    std::unique_ptr<sfz::Buffer<float>> buffer;

    fs::path filePath1;
    fs::path filePath2;
    fs::path filePath3;
};

#if defined(SFIZZ_USE_SNDFILE)
BENCHMARK_DEFINE_F(FileFixture, SndFile)(benchmark::State& state) {
    for (auto _ : state)
    {
        SndfileHandle sndfile(filePath1.c_str());
        buffer = absl::make_unique<sfz::Buffer<float>>(sndfile.channels() * sndfile.frames());
        sndfile.readf(buffer->data(), sndfile.frames());
    }
}
#endif

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

#if 0
BENCHMARK_DEFINE_F(FileFixture, LibNyquist)(benchmark::State& state) {
    for (auto _ : state)
    {
        nqr::AudioData data;
        nqr::NyquistIO loader;
        loader.Load(&data, filePath3.string());
        benchmark::DoNotOptimize(data);
    }
}
#endif

#if defined(SFIZZ_USE_SNDFILE)
BENCHMARK_REGISTER_F(FileFixture, SndFile);
#endif
BENCHMARK_REGISTER_F(FileFixture, DrWav);

#if 0
BENCHMARK_REGISTER_F(FileFixture, LibNyquist);
#endif
BENCHMARK_MAIN();
