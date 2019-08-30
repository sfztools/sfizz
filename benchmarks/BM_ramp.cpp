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
#include <random>
#include "../sfizz/SIMDHelpers.h"
#include "../sfizz/Buffer.h"


static void Dummy(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        benchmark::DoNotOptimize(value);
    }
}

static void LinearScalar(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        linearRamp<float, false>(absl::MakeSpan(output), 0.0f, value);
    }
}

static void LinearSIMD(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        linearRamp<float, true>(absl::MakeSpan(output), 0.0f, value);
    }
}
static void LinearScalarUnaligned(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        linearRamp<float, false>(absl::MakeSpan(output).subspan(1), 0.0f, value);
    }
}

static void LinearSIMDUnaligned(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        linearRamp<float, true>(absl::MakeSpan(output).subspan(1), 0.0f, value);
    }
}

static void MulScalar(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        multiplicativeRamp<float, false>(absl::MakeSpan(output), 1.0f, value);
    }
}

static void MulSIMD(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        multiplicativeRamp<float, true>(absl::MakeSpan(output), 1.0f, value);
    }
}
static void MulScalarUnaligned(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        multiplicativeRamp<float, false>(absl::MakeSpan(output).subspan(1), 1.0f, value);
    }
}

static void MulSIMDUnaligned(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        multiplicativeRamp<float, true>(absl::MakeSpan(output).subspan(1), 1.0f, value);
    }
}

// Register the function as a benchmark
BENCHMARK(Dummy)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LinearScalar)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LinearSIMD)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LinearScalarUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LinearSIMDUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(MulScalar)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(MulSIMD)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(MulScalarUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(MulSIMDUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK_MAIN();