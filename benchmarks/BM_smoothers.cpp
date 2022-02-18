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
#include "ModifierHelpers.h"
#include "Smoothers.h"
#include "absl/types/span.h"

class SmootherFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        input = std::vector<float>(state.range(0));
        output = std::vector<float>(state.range(0));
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
        sfz::cumsum<float>(input, absl::MakeSpan(input));
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }
    std::random_device rd {};
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0.5, 4 };
    std::vector<float> input;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(SmootherFixture, OnePole) (benchmark::State& state)
{
    sfz::OnePoleSmoother smoother;
    smoother.setSmoothing(10, sfz::config::defaultSampleRate);
    for (auto _ : state) {
        smoother.process(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(SmootherFixture, Linear) (benchmark::State& state)
{
    sfz::LinearSmoother smoother;
    smoother.setSmoothing(10, sfz::config::defaultSampleRate);
    for (auto _ : state) {
        smoother.process(input, absl::MakeSpan(output));
    }
}

BENCHMARK_REGISTER_F(SmootherFixture, OnePole)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(SmootherFixture, Linear)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
