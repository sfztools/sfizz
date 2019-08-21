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

static void Fill_float(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, false>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void Fill_float_unaligned(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, false>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

static void Fill_float_SSE(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, true>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void Fill_float_SSE_unaligned(benchmark::State& state) {
  Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    fill<float, true>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

static void Fill_double(benchmark::State& state) {
  Buffer<double> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<double> dist { 1, 2 };
  for (auto _ : state) {
    fill<double>(absl::MakeSpan(buffer), dist(gen));
  }
}

BENCHMARK(Dummy)->Range((2<<6), (2<<16));
BENCHMARK(Fill_float)->Range((2<<6), (2<<16));
BENCHMARK(Fill_float_SSE)->Range((2<<6), (2<<16));
BENCHMARK(Fill_float_unaligned)->Range((2<<6), (2<<16));
BENCHMARK(Fill_float_SSE_unaligned)->Range((2<<6), (2<<16));
BENCHMARK(Fill_double)->Range((2<<6), (2<<16));
BENCHMARK_MAIN();