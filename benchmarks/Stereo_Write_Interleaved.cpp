#include <benchmark/benchmark.h>
#include "../sources/StereoBuffer.h"
#include <algorithm>
#include <numeric>

static void Interleaved_Write(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  buffer.readInterleaved(input.data(), state.range(0));
  std::vector<float> output (state.range(0) * 2);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.writeInterleaved(output.data(), state.range(0));
  }
}
BENCHMARK(Interleaved_Write)->Range((8<<10) + 3, (8<<20) + 3);

static void Interleaved_Write_SSE(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  buffer.readInterleaved<true>(input.data(), state.range(0));
  std::vector<float> output (state.range(0) * 2);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.writeInterleaved<true>(output.data(), state.range(0));
  }
}
BENCHMARK(Interleaved_Write_SSE)->Range((8<<10) + 3, (8<<20) + 3);

static void Unaligned_Interleaved_Write(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  buffer.readInterleaved(input.data(), state.range(0));
  std::vector<float> output (state.range(0) * 2 + 1);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.writeInterleaved(output.data() + 1, state.range(0));
  }
}
BENCHMARK(Unaligned_Interleaved_Write)->Range((8<<10) + 3, (8<<20) + 3);

static void Unaligned_Interleaved_Write_SSE(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  buffer.readInterleaved<true>(input.data(), state.range(0));
  std::vector<float> output (state.range(0) * 2 + 1);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.writeInterleaved<true>(output.data() + 1, state.range(0));
  }
}
BENCHMARK(Unaligned_Interleaved_Write_SSE)->Range((8<<10) + 3, (8<<20) + 3);
BENCHMARK_MAIN();