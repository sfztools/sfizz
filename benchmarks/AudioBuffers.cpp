#include <benchmark/benchmark.h>
#include "../sources/AudioBuffer.h"
#include <algorithm>
#include <numeric>

static void Joint_Fill_float(benchmark::State& state) {
  AudioBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0f;
  }
}
// Register the function as a benchmark
BENCHMARK(Joint_Fill_float);

static void Split_Fill_float(benchmark::State& state) {
  SplitAudioBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0f;
  }
}
BENCHMARK(Split_Fill_float);

static void Joint_Fill_float_SSE(benchmark::State& state) {
  AudioBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill<VectorOperations::sse>(fillValue);
    fillValue += 1.0f;
  }
}
// Register the function as a benchmark
BENCHMARK(Joint_Fill_float_SSE);


static void Split_Fill_float_SSE(benchmark::State& state) {
  SplitAudioBuffer<float> buffer (100001);
  float fillValue = 0.0f;
  for (auto _ : state) {
    buffer.fill<VectorOperations::sse>(fillValue);
    fillValue += 1.0f;
  }
}

BENCHMARK(Split_Fill_float_SSE);

static void Joint_Fill_double(benchmark::State& state) {
  AudioBuffer<double> buffer (100001);
  double fillValue = 0.0;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0;
  }
}
// Register the function as a benchmark
BENCHMARK(Joint_Fill_double);

static void Split_Fill_double(benchmark::State& state) {
  SplitAudioBuffer<double> buffer (100001);
  double fillValue = 0.0;
  for (auto _ : state) {
    buffer.fill(fillValue);
    fillValue += 1.0;
  }
}

BENCHMARK(Split_Fill_double);

constexpr int size { 1039247 };
static void Interleaved_Read(benchmark::State& state) {
  AudioBuffer<float> buffer (size);
  std::array<float, 2*size> input;
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.readInterleaved(input.data(), size);
  }
}
BENCHMARK(Interleaved_Read);

static void Interleaved_Read_SSE(benchmark::State& state) {
  AudioBuffer<float> buffer (size);
  std::array<float, 2*size> input;
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.readInterleaved<VectorOperations::sse>(input.data(), size);
  }
}
BENCHMARK(Interleaved_Read_SSE);
BENCHMARK_MAIN();