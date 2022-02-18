// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "SIMDConfig.h"
#include <benchmark/benchmark.h>
#include <absl/types/span.h>
#include <random>
#include <algorithm>

#if SFIZZ_HAVE_SSE2
#include <emmintrin.h>
#endif

constexpr float filterGain { 0.25f };

void lowpass(absl::Span<const float> input, absl::Span<float> lowpass, float gain)
{
    float state = 0.0f;
    float intermediate;
    const auto G = gain / (1 - gain);
    auto in = input.begin();
    auto out = lowpass.begin();
    while (in < input.end()) {
        intermediate = G * (*in - state);
        *out = intermediate + state;
        state = *out + intermediate;
        in++;
        out++;
    }
}

void highpass(absl::Span<const float> input, absl::Span<float> highpass, float gain)
{
    float state = 0.0f;
    float intermediate;
    const auto G = gain / (1 - gain);
    auto in = input.begin();
    auto out = highpass.begin();
    while (in < input.end()) {
        intermediate = G * (*in - state);
        *out = *in - intermediate - state;
        state += 2*intermediate;
        in++;
        out++;
    }
}

void highpass_raw(absl::Span<const float> input, absl::Span<float> highpass, float gain)
{
    lowpass(input, highpass, gain);
    auto in = input.data();
    auto out = highpass.data();
    while (in < input.end()) {
        *out = *in - *out;
        out++;
        in++;
    }
}

void highpass_sse(absl::Span<const float> input, absl::Span<float> highpass, float gain)
{
    lowpass(input, highpass, gain);
    auto in = input.data();
    auto out = highpass.data();
    constexpr int FloatAlignment { 4 };
    const auto inputAlignedEnd = input.data() + (input.size() - (input.size() & (FloatAlignment - 1)));
    const auto outputAlignedEnd = highpass.data() + (highpass.size() - (highpass.size() & (FloatAlignment - 1)));
    while (in < inputAlignedEnd && out < outputAlignedEnd)
    {
        const auto inputRegister = _mm_loadu_ps(in);
        auto outputRegister = _mm_loadu_ps(out);
        outputRegister = _mm_sub_ps(inputRegister, outputRegister);
        _mm_storeu_ps(out, outputRegister);
        in += 4;
        out += 4;
    }
    while (in < input.end() && out < highpass.end())
    {
        *out = *in - *out;
        out++;
        in++;
    }
}


static void Low(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };

    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        lowpass(input, absl::MakeSpan(output), filterGain);
}

static void High(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };

    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass(input, absl::MakeSpan(output), filterGain);
}

static void High_Raw(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };

    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass_raw(input, absl::MakeSpan(output), filterGain);
}

static void High_SSE(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };

    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass_sse(input, absl::MakeSpan(output), filterGain);
}


BENCHMARK(Low)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High_Raw)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High_SSE)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK_MAIN();
