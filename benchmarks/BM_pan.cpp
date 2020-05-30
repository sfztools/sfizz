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
#include "Config.h"
#include "ScopedFTZ.h"
#include "absl/types/span.h"

class PanArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0.001f, 1.0f };
    pan = std::vector<float>(state.range(0));
    left = std::vector<float>(state.range(0));
    right = std::vector<float>(state.range(0));
    std::generate(pan.begin(), pan.end(), [&]() { return dist(gen); });
    std::generate(right.begin(), right.end(), [&]() { return dist(gen); });
    std::generate(left.begin(), left.end(), [&]() { return dist(gen); });
    temp1 = std::vector<float>(state.range(0));
    temp2 = std::vector<float>(state.range(0));
    span1 = absl::MakeSpan(temp1);
    span2 = absl::MakeSpan(temp2);
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  std::vector<float> pan;
  std::vector<float> left;
  std::vector<float> right;
  std::vector<float> temp1;
  std::vector<float> temp2;
  absl::Span<float> span1;
  absl::Span<float> span2;
};


BENCHMARK_DEFINE_F(PanArray, Scalar)(benchmark::State& state) {
    ScopedFTZ ftz;
    for (auto _ : state)
    {
        sfz::pan<float, false>(pan, absl::MakeSpan(left), absl::MakeSpan(right));
    }
}

BENCHMARK_DEFINE_F(PanArray, SIMD)(benchmark::State& state) {
    ScopedFTZ ftz;
    for (auto _ : state)
    {
        sfz::pan<float, true>(pan, absl::MakeSpan(left), absl::MakeSpan(right));
    }
}

BENCHMARK_REGISTER_F(PanArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(PanArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
