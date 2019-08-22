#include <benchmark/benchmark.h>
#include <random>
#include "../sources/SIMDHelpers.h"
#include "../sources/Buffer.h"


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