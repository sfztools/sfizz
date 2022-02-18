// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Buffer.h"
#include "AudioBuffer.h"
#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include "absl/memory/memory.h"
#include <samplerate.h>
#include <sndfile.hh>
#include "ghc/filesystem.hpp"
#include "hiir/Upsampler2xFpu.h"
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

template<bool SIMD=false>
void upsample2xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage2x.size()> upsampler;
    upsampler.set_coefs(coeffsStage2x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}


template<bool SIMD=false>
void upsample4xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage4x.size()> upsampler;
    upsampler.set_coefs(coeffsStage4x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

template<bool SIMD=false>
void upsample8xStage(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xFpu<coeffsStage8x.size()> upsampler;
    upsampler.set_coefs(coeffsStage8x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

#if defined(__x86_64__) || defined(__i386__)
#include "hiir/Upsampler2xSse.h"
template<>
void upsample2xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xSse<coeffsStage2x.size()> upsampler;
    upsampler.set_coefs(coeffsStage2x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

template<>
void upsample4xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xSse<coeffsStage4x.size()> upsampler;
    upsampler.set_coefs(coeffsStage4x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

template<>
void upsample8xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xSse<coeffsStage8x.size()> upsampler;
    upsampler.set_coefs(coeffsStage8x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

#elif defined(__arm__) || defined (__aarch64__)

#include "hiir/Upsampler2xNeon.h"

template<>
void upsample2xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xNeon<coeffsStage2x.size()> upsampler;
    upsampler.set_coefs(coeffsStage2x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

template<>
void upsample4xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xNeon<coeffsStage4x.size()> upsampler;
    upsampler.set_coefs(coeffsStage4x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

template<>
void upsample8xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2xNeon<coeffsStage8x.size()> upsampler;
    upsampler.set_coefs(coeffsStage8x.data());
    upsampler.process_block(output.data(), input.data(), static_cast<long>(input.size()));
}

#else

template<>
void upsample2xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    upsample2xStage<false>(input, output);
}
template<>
void upsample4xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    upsample4xStage<false>(input, output);
}
template<>
void upsample8xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    upsample8xStage<false>(input, output);
}
#endif

template <class T, bool SIMD=false>
std::unique_ptr<sfz::AudioBuffer<T>> upsample2x(const sfz::AudioBuffer<T>& buffer)
{
    // auto tempBuffer = absl::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto outputBuffer = absl::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 2);
    for (size_t channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}

template <class T, bool SIMD=false>
std::unique_ptr<sfz::AudioBuffer<T>> upsample4x(const sfz::AudioBuffer<T>& buffer)
{
    auto tempBuffer = absl::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto outputBuffer = absl::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 4);
    for (size_t channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), absl::MakeSpan(*tempBuffer));
        upsample4xStage<SIMD>(absl::MakeConstSpan(*tempBuffer), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}

template <class T, bool SIMD=false>
std::unique_ptr<sfz::AudioBuffer<T>> upsample8x(const sfz::AudioBuffer<T>& buffer)
{
    auto tempBuffer2x = absl::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 2);
    auto tempBuffer4x = absl::make_unique<sfz::Buffer<T>>(buffer.getNumFrames() * 4);
    auto outputBuffer = absl::make_unique<sfz::AudioBuffer<T>>(buffer.getNumChannels(), buffer.getNumFrames() * 8);
    for (size_t channelIdx = 0; channelIdx < buffer.getNumChannels(); channelIdx++) {
        upsample2xStage<SIMD>(buffer.getConstSpan(channelIdx), absl::MakeSpan(*tempBuffer2x));
        upsample4xStage<SIMD>(absl::MakeConstSpan(*tempBuffer2x), absl::MakeSpan(*tempBuffer4x));
        upsample8xStage<SIMD>(absl::MakeConstSpan(*tempBuffer4x), outputBuffer->getSpan(channelIdx));
    }
    return outputBuffer;
}


class SndFile : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /* state */)
    {

        const auto rootPath = getPath() / "sample1.wav";
        if (!ghc::filesystem::exists(rootPath)) {
        #ifndef NDEBUG
            std::cerr << "Can't find path" << '\n';
        #endif
            std::terminate();
        }

        SndfileHandle sndfile(rootPath.c_str());
        numFrames = sndfile.frames();
        numChannels = sndfile.channels();
        interleavedBuffer = absl::make_unique<sfz::Buffer<float>>(numChannels * numFrames);
        sndfile.readf(interleavedBuffer->data(), sndfile.frames());
    }

    void TearDown(const ::benchmark::State& /* state */)
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

    size_t numChannels;
    size_t numFrames;
    std::unique_ptr<sfz::Buffer<float>> interleavedBuffer;
};

BENCHMARK_DEFINE_F(SndFile, HIIR2X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample2x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR4X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample4x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR8X_scalar)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample8x<float, false>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR2X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample2x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR4X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample4x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR8X_vector)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample8x<float, true>(*baseBuffer);
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC2x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 2.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_FASTEST, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}


BENCHMARK_DEFINE_F(SndFile, SRC4x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC4x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC4x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 4.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_FASTEST, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_BEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_BEST_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_MEDIUM)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, SRC8x_FASTEST)(benchmark::State& state)
{
    for (auto _ : state) {
        auto intermediateBuffer = absl::make_unique<sfz::Buffer<float>>(2 * numChannels * numFrames);
        SRC_DATA srcData;
        srcData.data_in = interleavedBuffer->data();
        srcData.data_out = intermediateBuffer->data();
        srcData.src_ratio = 8.0;
        srcData.input_frames = static_cast<long>(numFrames);
        srcData.output_frames = static_cast<long>(2 * numFrames);
        src_simple(&srcData, SRC_SINC_FASTEST, static_cast<int>(numChannels));
        auto outBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, 2 * numFrames);
        sfz::readInterleaved(*intermediateBuffer, outBuffer->getSpan(0), outBuffer->getSpan(1));
        benchmark::DoNotOptimize(outBuffer);
    }
}

BENCHMARK_DEFINE_F(SndFile, HIIR8X_default)(benchmark::State& state)
{
    for (auto _ : state) {
        auto baseBuffer = absl::make_unique<sfz::AudioBuffer<float>>(numChannels, numFrames);
        sfz::readInterleaved(*interleavedBuffer, baseBuffer->getSpan(0), baseBuffer->getSpan(1));
        auto outBuffer = upsample8x<float>(*baseBuffer);
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
BENCHMARK_REGISTER_F(SndFile, HIIR8X_default);
BENCHMARK_MAIN();
