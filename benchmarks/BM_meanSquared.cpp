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
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

class MeanSquaredArray : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { 0, 1 };
        input = std::vector<float>(state.range(0));
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]])
    {
    }

    std::vector<float> input;
};

BENCHMARK_DEFINE_F(MeanSquaredArray, Scalar)
(benchmark::State& state)
{
    for (auto _ : state) {
        auto result = sfz::meanSquared<float, false>(input);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, SIMD)
(benchmark::State& state)
{
    for (auto _ : state) {
        auto result = sfz::meanSquared<float, true>(input);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, Scalar_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        auto result = sfz::meanSquared<float, false>(absl::MakeSpan(input).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MeanSquaredArray, SIMD_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        auto result = sfz::meanSquared<float, true>(absl::MakeSpan(input).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(MeanSquaredArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(MeanSquaredArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
