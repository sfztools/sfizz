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
#include "absl/types/span.h"
#include <benchmark/benchmark.h>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

class MyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { 0.1f, 1.0f };
        source = std::vector<float>(state.range(0));
        result = std::vector<float>(state.range(0));
        std::generate(source.begin(), source.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]])
    {
    }

    std::vector<float> source;
    std::vector<float> result;
};

BENCHMARK_DEFINE_F(MyFixture, Dummy)
(benchmark::State& state)
{
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            result[i] = source[i];
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, ScalarExp)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::exp<float, false>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, SIMDExp)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::exp<float, true>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, ScalarExp_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::exp<float, false>(absl::MakeSpan(source).subspan(1), absl::MakeSpan(result).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, SIMDExp_Unaligned)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::exp<float, true>(absl::MakeSpan(source).subspan(1), absl::MakeSpan(result).subspan(1));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, ScalarLog)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::log<float, false>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, SIMDLog)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::log<float, true>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, ScalarSin)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::sin<float, false>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, SIMDSin)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::sin<float, true>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, ScalarCos)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::cos<float, false>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(MyFixture, SIMDCos)
(benchmark::State& state)
{
    for (auto _ : state) {
        sfz::cos<float, true>(source, absl::MakeSpan(result));
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(MyFixture, Dummy)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, ScalarExp)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, SIMDExp)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, ScalarExp_Unaligned)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, SIMDExp_Unaligned)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, ScalarLog)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, SIMDLog)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, ScalarSin)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, SIMDSin)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, ScalarCos)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);
BENCHMARK_REGISTER_F(MyFixture, SIMDCos)->RangeMultiplier(4)->Range(1 << 6, 1 << 10);

BENCHMARK_MAIN();
