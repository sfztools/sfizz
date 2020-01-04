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

#include "Config.h"
#include <benchmark/benchmark.h>
#include <absl/types/span.h>
#include <random>
#include <algorithm>

#if HAVE_X86INTRIN_H
#include <x86intrin.h>
#endif

#if HAVE_INTRIN_H
#include <intrin.h>
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
