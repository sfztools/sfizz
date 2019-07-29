#include <benchmark/benchmark.h>
#include "../sources/AudioBuffer.h"
#include <algorithm>

static void AB_Joint_Fill_float(benchmark::State& state) {
  AudioBuffer<float> buffer (65535);
  float fillValue = 0.0f;
  for (auto _ : state) {
    std::fill(buffer.begin(0), buffer.end(0), fillValue);
    std::fill(buffer.begin(1), buffer.end(1), fillValue);
    fillValue += 1.0f;
  }
}
// Register the function as a benchmark
BENCHMARK(AB_Joint_Fill_float);

static void AB_Split_Fill_float(benchmark::State& state) {
  SplitAudioBuffer<float> buffer (65535);
  float fillValue = 0.0f;
  for (auto _ : state) {
    std::fill(buffer.begin(0), buffer.end(0), fillValue);
    std::fill(buffer.begin(1), buffer.end(1), fillValue);
    fillValue += 1.0f;
  }
}
BENCHMARK(AB_Split_Fill_float);

static void AB_Joint_Fill_double(benchmark::State& state) {
  AudioBuffer<double> buffer (65535);
  double fillValue = 0.0;
  for (auto _ : state) {
    std::fill(buffer.begin(0), buffer.end(0), fillValue);
    std::fill(buffer.begin(1), buffer.end(1), fillValue);
    fillValue += 1.0;
  }
}
// Register the function as a benchmark
BENCHMARK(AB_Joint_Fill_double);

static void AB_Split_Fill_double(benchmark::State& state) {
  SplitAudioBuffer<double> buffer (65535);
  double fillValue = 0.0;
  for (auto _ : state) {
    std::fill(buffer.begin(0), buffer.end(0), fillValue);
    std::fill(buffer.begin(1), buffer.end(1), fillValue);
    fillValue += 1.0;
  }
}
BENCHMARK(AB_Split_Fill_double);