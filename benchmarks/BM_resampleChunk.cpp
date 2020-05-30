// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Buffer.h"
#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#include "ghc/filesystem.hpp"
#include "Oversampler.h"
#include "AudioBuffer.h"
#include "absl/memory/memory.h"
#include <iostream>


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


#if defined(__x86_64__) || defined(__i386__)
#include "hiir/Upsampler2xSse.h"
using Upsampler2x = hiir::Upsampler2xSse<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xSse<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xSse<coeffsStage8x.size()>;
#elif defined(__arm__) || defined(__aarch64__)
#include "hiir/Upsampler2xNeon.h"
using Upsampler2x = hiir::Upsampler2xNeon<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xNeon<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xNeon<coeffsStage8x.size()>;
#else
#include "hiir/Upsampler2xFpu.h"
using Upsampler2x = hiir::Upsampler2xFpu<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xFpu<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xFpu<coeffsStage8x.size()>;
#endif


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
        output = absl::make_unique<sfz::AudioBuffer<float>>(sndfile.channels(), numFrames * 4);
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
        sfz::readInterleaved(buffer, output->getSpan(0), output->getSpan(1));
    }
}

BENCHMARK_DEFINE_F(FileFixture, ResampleAtOnce)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Buffer<float> buffer { numFrames * sndfile.channels() };
        sfz::Buffer<float> temp  { numFrames * 4 };

        Upsampler2x upsampler2x;
        Upsampler4x upsampler4x;
        upsampler2x.set_coefs(coeffsStage2x.data());
        upsampler4x.set_coefs(coeffsStage4x.data());

        sndfile.readf(buffer.data(), numFrames);
        sfz::readInterleaved(buffer, output->getSpan(0), output->getSpan(1));

        upsampler2x.process_block(temp.data(), output->channelReader(0), static_cast<long>(numFrames));
        upsampler4x.process_block(output->channelWriter(0), temp.data(), static_cast<long>(numFrames * 2));

        upsampler2x.clear_buffers();
        upsampler4x.clear_buffers();

        upsampler2x.process_block(temp.data(), output->channelReader(1), static_cast<long>(numFrames));
        upsampler4x.process_block(output->channelWriter(1), temp.data(), static_cast<long>(numFrames * 2));
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

        Upsampler2x upsampler2xLeft;
        Upsampler2x upsampler2xRight;
        Upsampler4x upsampler4xLeft;
        Upsampler4x upsampler4xRight;
        upsampler2xLeft.set_coefs(coeffsStage2x.data());
        upsampler2xRight.set_coefs(coeffsStage2x.data());
        upsampler4xLeft.set_coefs(coeffsStage4x.data());
        upsampler4xRight.set_coefs(coeffsStage4x.data());

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

            sfz::readInterleaved(bufferChunk, leftSpan, rightSpan);

            upsampler2xLeft.process_block(chunkSpan.data(), leftSpan.data(), static_cast<long>(thisChunkSize));
            upsampler4xLeft.process_block(output->channelWriter(0) + outputFrameCounter, chunkSpan.data(), static_cast<long>(thisChunkSize * 2));

            upsampler2xRight.process_block(chunkSpan.data(), rightSpan.data(), static_cast<long>(thisChunkSize));
            upsampler4xRight.process_block(output->channelWriter(1) + outputFrameCounter, chunkSpan.data(), static_cast<long>(thisChunkSize * 2));

            inputFrameCounter += chunkSize;
            outputFrameCounter += chunkSize * 4;
        }
    }
}

BENCHMARK_REGISTER_F(FileFixture, NoResampling);
BENCHMARK_REGISTER_F(FileFixture, ResampleAtOnce);
BENCHMARK_REGISTER_F(FileFixture, ResampleInChunks)->RangeMultiplier(4)->Range((1 << 4), (1 << 16));
BENCHMARK_MAIN();
