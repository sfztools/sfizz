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

class ClampArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 10 };
    input = std::vector<float>(state.range(0));
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& state) {
      UNUSED(state);
  }

  std::vector<float> input;
};

BENCHMARK_DEFINE_F(ClampArray, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, false);
        sfz::clampAll<float>(absl::MakeSpan(input), 1.2f, 3.8f);
    }
}

BENCHMARK_DEFINE_F(ClampArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::setSIMDOpStatus<float>(sfz::SIMDOps::clampAll, true);
        sfz::clampAll<float>(absl::MakeSpan(input), 1.2f, 3.8f);
    }
}


BENCHMARK_REGISTER_F(ClampArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(ClampArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
