// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "BM_opcodeSpec.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>

class OpcodeSpecFixture : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0.0f, 1.0f };
    value = dist(gen);
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  float value;
  float returned;
};

BENCHMARK_DEFINE_F(OpcodeSpecFixture, ConstexprClamp)(benchmark::State& state) {
    for (auto _ : state)
    {
        if (constexprSpec.flags | (1 << 2))
            returned = constexprSpec.bounds.clamp(value);
        benchmark::DoNotOptimize(returned);
    }
}

BENCHMARK_DEFINE_F(OpcodeSpecFixture, ConstexprDontClamp)(benchmark::State& state) {
    for (auto _ : state)
    {
        if (constexprSpec.flags | (1 << 1))
            returned = constexprSpec.bounds.clamp(value);
        benchmark::DoNotOptimize(returned);
    }
}

BENCHMARK_DEFINE_F(OpcodeSpecFixture, ConstClamp)(benchmark::State& state) {
    for (auto _ : state)
    {
        if (constSpec.flags | (1 << 2))
            returned = constSpec.bounds.clamp(value);
        benchmark::DoNotOptimize(returned);
    }
}

BENCHMARK_DEFINE_F(OpcodeSpecFixture, ConstDontClamp)(benchmark::State& state) {
    for (auto _ : state)
    {
        if (constSpec.flags | (1 << 1))
            returned = constSpec.bounds.clamp(value);
        benchmark::DoNotOptimize(returned);
    }
}

BENCHMARK_REGISTER_F(OpcodeSpecFixture, ConstexprClamp);
BENCHMARK_REGISTER_F(OpcodeSpecFixture, ConstexprDontClamp);
BENCHMARK_REGISTER_F(OpcodeSpecFixture, ConstClamp);
BENCHMARK_REGISTER_F(OpcodeSpecFixture, ConstDontClamp);
BENCHMARK_MAIN();
