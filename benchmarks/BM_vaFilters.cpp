// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "OnePoleFilter.h"
#include "SfzFilter.h"
#include "ScopedFTZ.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>

constexpr int blockSize { 256 };
constexpr float sampleRate { 48000.0f };

class FilterFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&)
    {
        input = std::vector<float>(blockSize);
        output = std::vector<float>(blockSize);
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }
    std::random_device rd {};
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { 1, 0.2 };
    std::vector<float> input;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(FilterFixture, OnePole_VA) (benchmark::State& state)
{
    ScopedFTZ ftz;
    sfz::OnePoleFilter<float> filter;
    for (auto _ : state) {
        filter.processLowpass(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(FilterFixture, OnePoleMul_VA) (benchmark::State& state)
{
    ScopedFTZ ftz;
    sfz::OnePoleFilterMul<float> filter;
    for (auto _ : state) {
        filter.processLowpass(input, absl::MakeSpan(output));
    }
}
BENCHMARK_REGISTER_F(FilterFixture, OnePole_VA);
BENCHMARK_REGISTER_F(FilterFixture, OnePoleMul_VA);
BENCHMARK_MAIN();
