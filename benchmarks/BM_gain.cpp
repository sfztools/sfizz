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

class GainSingle : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    input = std::vector<float>(state.range(0));
    output = std::vector<float>(state.range(0));
    gain = dist(gen);
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  float gain;
  std::vector<float> input;
  std::vector<float> output;
};

class GainArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    input = std::vector<float>(state.range(0));
    output = std::vector<float>(state.range(0));
    gain = std::vector<float>(state.range(0));
    std::generate(gain.begin(), gain.end(), [&]() { return dist(gen); });
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  std::vector<float> gain;
  std::vector<float> input;
  std::vector<float> output;
};

BENCHMARK_DEFINE_F(GainSingle, Straight)(benchmark::State& state) {
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = gain * input[i];
    }
}

BENCHMARK_DEFINE_F(GainSingle, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, false);
        sfz::applyGain1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainSingle, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain1, true);
        sfz::applyGain1<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainArray, Straight)(benchmark::State& state) {
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = gain[i] * input[i];
    }
}

BENCHMARK_DEFINE_F(GainArray, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, false);
        sfz::applyGain<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, true);
        sfz::applyGain<float>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainArray, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, false);
        sfz::applyGain<float>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(GainArray, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::gain, true);
        sfz::applyGain<float>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}



BENCHMARK_REGISTER_F(GainSingle, Straight)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainSingle, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainSingle, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainArray, Straight)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(GainArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
