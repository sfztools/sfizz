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

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <numeric>
#include <absl/algorithm/container.h>
#include "SIMDHelpers.h"

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

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

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