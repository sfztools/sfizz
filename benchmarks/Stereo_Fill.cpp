#include <benchmark/benchmark.h>
#include "../sources/StereoBuffer.h"
#include <algorithm>
#include <numeric>

static void Fill_float(benchmark::State& state) {
  StereoBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0f;
  }
}
// Register the function as a benchmark
BENCHMARK(Fill_float);

static void Fill_float_SSE(benchmark::State& state) {
  StereoBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill<SIMD::sse>(fillValue);
    fillValue += 1.0f;
  }
}
// Register the function as a benchmark
BENCHMARK(Fill_float_SSE);

static void Fill_double(benchmark::State& state) {
  StereoBuffer<double> buffer (100001);
  double fillValue = 0.0;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0;
  }
}
// Register the function as a benchmark
BENCHMARK(Fill_double);

BENCHMARK_MAIN();