// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <benchmark/benchmark.h>
#include <chrono>

class Clock : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& /* state */) {

  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

  std::chrono::time_point<std::chrono::high_resolution_clock> hires;
  std::chrono::time_point<std::chrono::steady_clock> steady;
};


BENCHMARK_DEFINE_F(Clock, HighRes)(benchmark::State& state) {
    for (auto _ : state)
    {
        hires = std::chrono::high_resolution_clock::now();
    }
}

BENCHMARK_DEFINE_F(Clock, Steady)(benchmark::State& state) {
    for (auto _ : state)
    {
        steady = std::chrono::steady_clock::now();
    }
}

// Just checking that stuff happens..
BENCHMARK_DEFINE_F(Clock, Both)(benchmark::State& state) {
    for (auto _ : state)
    {
        hires = std::chrono::high_resolution_clock::now();
        steady = std::chrono::steady_clock::now();
    }
}

BENCHMARK_REGISTER_F(Clock, HighRes);
BENCHMARK_REGISTER_F(Clock, Steady);
BENCHMARK_REGISTER_F(Clock, Both);
BENCHMARK_MAIN();
