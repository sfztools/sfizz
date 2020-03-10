// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <benchmark/benchmark.h>
#include "Logger.h"
#include <chrono>

class Logger : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& /* state */) {

  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

};

BENCHMARK_DEFINE_F(Logger, Baseline)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Logger logger {};
        benchmark::DoNotOptimize(logger);
    }
}

BENCHMARK_DEFINE_F(Logger, ProcessingTime)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::Logger logger {};
        sfz::CallbackBreakdown breakdown;
        breakdown.data = sfz::Duration(1);
        breakdown.renderMethod = sfz::Duration(1);
        breakdown.dispatch = sfz::Duration(1);
        breakdown.panning = sfz::Duration(1);
        breakdown.filters = sfz::Duration(1);
        breakdown.amplitude = sfz::Duration(1);
        logger.logCallbackTime(std::move(breakdown), 16, 16);
    }
}

BENCHMARK_REGISTER_F(Logger, Baseline);
BENCHMARK_REGISTER_F(Logger, ProcessingTime);
BENCHMARK_MAIN();
