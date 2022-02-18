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

class MultiplyAdd : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    input = std::vector<float>(state.range(0));
    output = std::vector<float>(state.range(0));
    gain = std::vector<float>(state.range(0));
    std::fill(output.begin(), output.end(), 1.0f );
    std::generate(gain.begin(), gain.end(), [&]() { return dist(gen); });
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  std::vector<float> gain;
  std::vector<float> input;
  std::vector<float> output;
};

BENCHMARK_DEFINE_F(MultiplyAdd, Straight)(benchmark::State& state) {
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
            output[i] += gain[i] * input[i];
    }
}

BENCHMARK_DEFINE_F(MultiplyAdd, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, false);
        sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyAdd, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, true);
        sfz::multiplyAdd<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(MultiplyAdd, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, false);
        sfz::multiplyAdd<float>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(MultiplyAdd, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::multiplyAdd, true);
        sfz::multiplyAdd<float>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(MultiplyAdd, Straight)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAdd, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAdd, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAdd, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MultiplyAdd, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
