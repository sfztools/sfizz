// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "BM.h"
#include "Buffer.h"
#include <algorithm>
#include <random>
#include <numeric>

static void Dummy(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    auto fillValue = dist(gen);
    benchmark::DoNotOptimize(fillValue);
  }
}

static void FillScalar(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, false>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillScalar_unaligned(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, false>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

static void FillSIMD(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, true>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillSIMD_unaligned(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, true>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

BENCHMARK(Dummy)->RangeMultiplier(4)->Range((1<<2), (1<<12));
BENCHMARK(FillScalar)->RangeMultiplier(4)->Range((1<<2), (1<<12));
BENCHMARK(FillSIMD)->RangeMultiplier(4)->Range((1<<2), (1<<12));
BENCHMARK(FillScalar_unaligned)->RangeMultiplier(4)->Range((1<<2), (1<<12));
BENCHMARK(FillSIMD_unaligned)->RangeMultiplier(4)->Range((1<<2), (1<<12));
BENCHMARK_MAIN();
