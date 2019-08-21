#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>
#include "../sources/SIMDHelpers.h"

static void Dummy(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::vector<float> source(state.range(0));
    std::vector<float> result(state.range(0));
    std::normal_distribution<float> dist { };
    std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
    for (auto _ : state)
    {
         for (int i = 0; i < state.range(0); ++i)
            result[i] = source[i];
        benchmark::DoNotOptimize(result);
    }
}

static void StdExp(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::vector<float> source(state.range(0));
    std::vector<float> result(state.range(0));
    std::normal_distribution<float> dist { };
    std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
            result[i] = std::exp(source[i]);
        benchmark::DoNotOptimize(result);
    }
}

// static void StdExpOMP(benchmark::State& state) {
//     std::random_device rd { };
//     std::mt19937 gen { rd() };
//     std::vector<float> source(state.range(0));
//     std::vector<float> result(state.range(0));
//     std::normal_distribution<float> dist { };
//     std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
//     for (auto _ : state)
//     {
//         #pragma omp simd
//         for (int i = 0; i < state.range(0); ++i)
//             result[i] = std::exp(source[i]);
//         benchmark::DoNotOptimize(result);
//     }
// }

static void Scalar(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::vector<float> source(state.range(0));
    std::vector<float> result(state.range(0));
    std::normal_distribution<float> dist { };
    std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
    for (auto _ : state)
    {
        exp<float, false>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

static void SIMD(benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::vector<float> source(state.range(0));
    std::vector<float> result(state.range(0));
    std::normal_distribution<float> dist { };
    std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
    for (auto _ : state)
    {
        exp<float, true>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(Dummy)->RangeMultiplier(2)->Range(1 << 6, 1 << 10);
BENCHMARK(StdExp)->RangeMultiplier(2)->Range(1 << 6, 1 << 10);
BENCHMARK(Scalar)->RangeMultiplier(2)->Range(1 << 6, 1 << 10);
BENCHMARK(SIMD)->RangeMultiplier(2)->Range(1 << 6, 1 << 10);
BENCHMARK_MAIN();