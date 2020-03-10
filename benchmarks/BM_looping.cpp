// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <benchmark/benchmark.h>
#include "SIMDHelpers.h"
#include <vector>
#include <random>
#include <numeric>
#include <absl/algorithm/container.h>

// In this one we have an array of indices

constexpr int loopStart { 5 };
constexpr int loopEnd { 1076 };
constexpr float maxJump { 4 };

class LoopingFixture : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, maxJump };
    indices = std::vector<int>(state.range(0));
    leftCoeffs = std::vector<float>(state.range(0));
    rightCoeffs = std::vector<float>(state.range(0));
    jumps = std::vector<float>(state.range(0));
    absl::c_generate(jumps, [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

    std::vector<int> indices;
    std::vector<float> leftCoeffs;
    std::vector<float> rightCoeffs;
    std::vector<float> jumps;
};


BENCHMARK_DEFINE_F(LoopingFixture, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::loopingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 2.5f, loopEnd, loopStart);
    }
}

BENCHMARK_DEFINE_F(LoopingFixture, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::loopingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 2.5f, loopEnd, loopStart);
    }
}

BENCHMARK_DEFINE_F(LoopingFixture, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::loopingSFZIndex<float, false>(absl::MakeSpan(jumps).subspan(1), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1), absl::MakeSpan(indices).subspan(3), 2.5f, loopEnd, loopStart);
    }
}

BENCHMARK_DEFINE_F(LoopingFixture, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::loopingSFZIndex<float, true>(absl::MakeSpan(jumps).subspan(1), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1), absl::MakeSpan(indices).subspan(3), 2.5f, loopEnd, loopStart);
    }
}


// Register the function as a benchmark
BENCHMARK_REGISTER_F(LoopingFixture, Scalar)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(LoopingFixture, SIMD)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(LoopingFixture, Scalar_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(LoopingFixture, SIMD_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();
