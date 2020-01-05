// Copyright (c) 2019, Paul Ferrand
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
#include "Config.h"
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

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

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
    for (auto _ : state)
    {
        sfz::pan<float, false>(pan, absl::MakeSpan(left), absl::MakeSpan(right));
    }
}

BENCHMARK_DEFINE_F(PanArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::pan<float, true>(pan, absl::MakeSpan(left), absl::MakeSpan(right));
    }
}

BENCHMARK_DEFINE_F(PanArray, BlockOps)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::fill<float>(span2, 1.0f);
        sfz::add<float>(span1, span2);
        sfz::applyGain<float>(piFour<float>, span2);
        sfz::cos<float>(span2, span1);
        sfz::sin<float>(span2, span2);
        sfz::applyGain<float>(span1, absl::MakeSpan(left));
        sfz::applyGain<float>(span2, absl::MakeSpan(right));
    }
}

BENCHMARK_REGISTER_F(PanArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(PanArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(PanArray, BlockOps)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
