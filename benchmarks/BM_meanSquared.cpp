// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

class MeanSquaredArray : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { 0, 1 };
        input = std::vector<float>(state.range(0));
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    std::vector<float> input;
};

BENCHMARK_DEFINE_F(MeanSquaredArray, Scalar)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, false);
        auto result = sfz::meanSquared<float>(input);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, SIMD)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, true);
        auto result = sfz::meanSquared<float>(input);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, Scalar_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, false);
        auto result = sfz::meanSquared<float>(absl::MakeSpan(input).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, SIMD_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::sumSquares, true);
        auto result = sfz::meanSquared<float>(absl::MakeSpan(input).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(MeanSquaredArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
