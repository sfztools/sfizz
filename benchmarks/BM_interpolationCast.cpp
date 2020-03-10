// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <numeric>
#include <absl/algorithm/container.h>

// In this one we have an array of jumps

constexpr float maxJump { 4 };

class InterpolationCast : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, maxJump };
    jumps = std::vector<int>(state.range(0));
    leftCoeffs = std::vector<float>(state.range(0));
    rightCoeffs = std::vector<float>(state.range(0));
    floatJumps = std::vector<float>(state.range(0));
    absl::c_generate(floatJumps, [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

    std::vector<int> jumps;
    std::vector<float> leftCoeffs;
    std::vector<float> rightCoeffs;
    std::vector<float> floatJumps;
};


BENCHMARK_DEFINE_F(InterpolationCast, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::sfzInterpolationCast<float, false>(floatJumps, absl::MakeSpan(jumps), absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs));
    }
}

BENCHMARK_DEFINE_F(InterpolationCast, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::sfzInterpolationCast<float, true>(floatJumps, absl::MakeSpan(jumps), absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs));
    }
}

BENCHMARK_DEFINE_F(InterpolationCast, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::sfzInterpolationCast<float, false>(absl::MakeSpan(floatJumps).subspan(1), absl::MakeSpan(jumps).subspan(3), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1));
    }
}

BENCHMARK_DEFINE_F(InterpolationCast, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::sfzInterpolationCast<float, true>(absl::MakeSpan(floatJumps).subspan(1), absl::MakeSpan(jumps).subspan(3), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1));
    }
}


// Register the function as a benchmark
BENCHMARK_REGISTER_F(InterpolationCast, Scalar)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(InterpolationCast, SIMD)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(InterpolationCast, Scalar_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(InterpolationCast, SIMD_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();
