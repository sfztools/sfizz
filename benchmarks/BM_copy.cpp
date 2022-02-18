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

class CopyArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    input = std::vector<float>(state.range(0));
    output = std::vector<float>(state.range(0));
    std::generate(output.begin(), output.end(), [&]() { return dist(gen); });
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  std::vector<float> input;
  std::vector<float> output;
};


BENCHMARK_DEFINE_F(CopyArray, StdCopy)(benchmark::State& state) {
    for (auto _ : state)
    {
        std::copy(input.begin(), input.end(), output.begin());
    }
}

BENCHMARK_DEFINE_F(CopyArray, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, false);
        sfz::copy<float>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(CopyArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, true);
        sfz::copy<float>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(CopyArray, StdCopy_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        std::copy(input.begin() + 1, input.end(), output.begin() + 1);
    }
}

BENCHMARK_DEFINE_F(CopyArray, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, false);
        sfz::copy<float>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(CopyArray, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::copy, true);
        sfz::copy<float>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(CopyArray, StdCopy)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_REGISTER_F(CopyArray, Scalar)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_REGISTER_F(CopyArray, SIMD)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_REGISTER_F(CopyArray, StdCopy_Unaligned)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_REGISTER_F(CopyArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_REGISTER_F(CopyArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 4, 1 << 16);
BENCHMARK_MAIN();
