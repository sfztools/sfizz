// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "BM.h"
#include <random>
#include "Buffer.h"


static void Dummy(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
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
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, false>(absl::MakeSpan(output), 0.0f, value);
    }
}

static void LinearSIMD(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, true>(absl::MakeSpan(output), 0.0f, value);
    }
}
static void LinearScalarUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, false>(absl::MakeSpan(output).subspan(1), 0.0f, value);
    }
}

static void LinearSIMDUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, true>(absl::MakeSpan(output).subspan(1), 0.0f, value);
    }
}

static void MulScalar(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::multiplicativeRamp<float, false>(absl::MakeSpan(output), 1.0f, value);
    }
}

static void MulSIMD(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::multiplicativeRamp<float, true>(absl::MakeSpan(output), 1.0f, value);
    }
}
static void MulScalarUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::multiplicativeRamp<float, false>(absl::MakeSpan(output).subspan(1), 1.0f, value);
    }
}

static void MulSIMDUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::multiplicativeRamp<float, true>(absl::MakeSpan(output).subspan(1), 1.0f, value);
    }
}

static void LogDomainScalar(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, false>(absl::MakeSpan(output), 1.0f, value);
        sfz::applyGain<float, false>(std::log(2.0f), absl::MakeSpan(output));
        sfz::exp<float, false>(output, absl::MakeSpan(output));
    }
}

static void LogDomainSIMD(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        sfz::linearRamp<float, true>(absl::MakeSpan(output), 1.0f, value);
        sfz::applyGain<float, true>(std::log(2.0f), absl::MakeSpan(output));
        sfz::exp<float, true>(output, absl::MakeSpan(output));
    }
}
static void LogDomainScalarUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        auto outputSpan = absl::MakeSpan(output).subspan(1);
        sfz::linearRamp<float, false>(outputSpan, 1.0f, value);
        sfz::applyGain<float, false>(std::log(2.0f), outputSpan);
        sfz::exp<float, false>(outputSpan, outputSpan);
    }
}

static void LogDomainSIMDUnaligned(benchmark::State& state) {
    sfz::Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 2 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        auto outputSpan = absl::MakeSpan(output).subspan(1);
        sfz::linearRamp<float, true>(outputSpan, 1.0f, value);
        sfz::applyGain<float, true>(std::log(2.0f), outputSpan);
        sfz::exp<float, true>(outputSpan, outputSpan);
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
BENCHMARK(LogDomainScalar)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LogDomainSIMD)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LogDomainScalarUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK(LogDomainSIMDUnaligned)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK_MAIN();
