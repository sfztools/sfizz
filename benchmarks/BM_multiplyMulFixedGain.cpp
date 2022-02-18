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

class MultiplyMulFixedGain : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { 0.1f, 1.0f };
        input = std::vector<float>(state.range(0));
        output = std::vector<float>(state.range(0));
        gain = dist(gen);
        std::fill(output.begin(), output.end(), 2.0f);
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    float gain = {};
    std::vector<float> input;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(MultiplyMulFixedGain, Straight)
(benchmark::State& state)
{
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] *= gain * input[i];
    }
}

BENCHMARK_DEFINE_F(MultiplyMulFixedGain, Scalar)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, false);
        sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyMulFixedGain, SIMD)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, true);
        sfz::multiplyMul1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyMulFixedGain, Scalar_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, false);
        sfz::multiplyMul1<float>(gain, absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(MultiplyMulFixedGain, SIMD_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyMul1, true);
        sfz::multiplyMul1<float>(gain, absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(MultiplyMulFixedGain, Straight)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyMulFixedGain, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyMulFixedGain, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyMulFixedGain, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyMulFixedGain, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
