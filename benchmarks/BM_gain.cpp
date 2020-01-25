// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
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

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

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

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

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
        sfz::applyGain<float, false>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainSingle, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::applyGain<float, true>(gain, input, absl::MakeSpan(output));
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
        sfz::applyGain<float, false>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::applyGain<float, true>(gain, input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(GainArray, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::applyGain<float, false>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(GainArray, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::applyGain<float, true>(absl::MakeSpan(gain).subspan(1), absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
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
