// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "utility/Macros.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <iostream>
#include <cmath>

class AddArray : public benchmark::Fixture {
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

  void TearDown(const ::benchmark::State& state) {
      UNUSED(state);
  }

  std::vector<float> input;
  std::vector<float> output;
};

BENCHMARK_DEFINE_F(AddArray, Value_Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add1, false);
        sfz::add1<float>(1.1f, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, Value_SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add1, true);
        sfz::add1<float>(1.1f, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, Value_Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add1, false);
        sfz::add1<float>(1.1f, absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(AddArray, Value_SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add1, true);
        sfz::add1<float>(1.1f, absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(AddArray, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, false);
        sfz::add<float>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, true);
        sfz::add<float>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, false);
        sfz::add<float>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(AddArray, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::add, true);
        sfz::add<float>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(AddArray, Value_Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Value_SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Value_Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Value_SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
