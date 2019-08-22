#include <benchmark/benchmark.h>
#include "../sources/SIMDHelpers.h"
#include "../sources/Buffer.h"
#include <algorithm>
#include <random>
#include <numeric>

static void Dummy(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    auto fillValue = dist(gen);
    benchmark::DoNotOptimize(fillValue);
  }
}

static void FillScalar(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, false>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillScalar_unaligned(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, false>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

static void FillSIMD(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, true>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillSIMD_unaligned(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, true>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

BENCHMARK(Dummy)->Range((2<<6), (2<<16));
BENCHMARK(FillScalar)->Range((2<<6), (2<<16));
BENCHMARK(FillSIMD)->Range((2<<6), (2<<16));
BENCHMARK(FillScalar_unaligned)->Range((2<<6), (2<<16));
BENCHMARK(FillSIMD_unaligned)->Range((2<<6), (2<<16));
BENCHMARK_MAIN();