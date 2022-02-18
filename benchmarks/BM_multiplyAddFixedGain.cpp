// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>

class MultiplyAddFixedGain : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { 0, 1 };
        input = std::vector<float>(state.range(0));
        output = std::vector<float>(state.range(0));
        gain = dist(gen);
        std::fill(output.begin(), output.end(), 1.0f);
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    float gain = {};
    std::vector<float> input;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(MultiplyAddFixedGain, Straight)
(benchmark::State& state)
{
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] += gain * input[i];
    }
}

BENCHMARK_DEFINE_F(MultiplyAddFixedGain, Scalar)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, false);
        sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyAddFixedGain, SIMD)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, true);
        sfz::multiplyAdd1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyAddFixedGain, Scalar_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, false);
        sfz::multiplyAdd1<float>(gain, absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(MultiplyAddFixedGain, SIMD_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd1, true);
        sfz::multiplyAdd1<float>(gain, absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(MultiplyAddFixedGain, Straight)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAddFixedGain, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAddFixedGain, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAddFixedGain, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAddFixedGain, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
