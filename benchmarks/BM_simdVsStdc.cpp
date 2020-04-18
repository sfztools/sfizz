// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <cstdlib>

class MyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        src.resize(state.range(0));
        dst.resize(state.range(0));
    }

    std::vector<float> src;
    std::vector<float> dst;
};

BENCHMARK_DEFINE_F(MyFixture, SimdFill)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::fill(absl::MakeSpan(dst), 0.0f);
    }
}

BENCHMARK_DEFINE_F(MyFixture, StdcFill)(benchmark::State& state) {
    for (auto _ : state)
    {
        std::memset(dst.data(), 0, dst.size() * sizeof(dst[0]));
    }
}

BENCHMARK_DEFINE_F(MyFixture, SimdCopy)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::copy(absl::MakeConstSpan(src), absl::MakeSpan(dst));
    }
}

BENCHMARK_DEFINE_F(MyFixture, StdcCopy)(benchmark::State& state) {
    for (auto _ : state)
    {
        std::memcpy(dst.data(), src.data(), dst.size() * sizeof(dst[0]));
    }
}

BENCHMARK_REGISTER_F(MyFixture, SimdFill)->RangeMultiplier(4)->Range(1, 1024 * 1024);
BENCHMARK_REGISTER_F(MyFixture, StdcFill)->RangeMultiplier(4)->Range(1, 1024 * 1024);
BENCHMARK_REGISTER_F(MyFixture, SimdCopy)->RangeMultiplier(4)->Range(1, 1024 * 1024);
BENCHMARK_REGISTER_F(MyFixture, StdcCopy)->RangeMultiplier(4)->Range(1, 1024 * 1024);
BENCHMARK_MAIN();
