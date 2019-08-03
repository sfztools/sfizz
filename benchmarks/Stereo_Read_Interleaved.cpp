#include <benchmark/benchmark.h>
#include "../sources/StereoBuffer.h"
#include <algorithm>
#include <numeric>

static void Interleaved_Read(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.readInterleaved(input.data(), state.range(0));
  }
}
BENCHMARK(Interleaved_Read)->Range((8<<10) + 3, (8<<20) + 3);

static void Interleaved_Read_SSE(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.readInterleaved<SIMD::sse>(input.data(), state.range(0));
  }
}
BENCHMARK(Interleaved_Read_SSE)->Range((8<<10) + 3, (8<<20) + 3);

static void Unaligned_Interleaved_Read_SSE(benchmark::State& state) {
  StereoBuffer<float> buffer (state.range(0));
  std::vector<float> input (state.range(0) * 2);
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    buffer.readInterleaved<SIMD::sse>(input.data() + 1, state.range(0) - 1);
  }
}
BENCHMARK(Unaligned_Interleaved_Read_SSE)->Range((8<<10) + 3, (8<<20) + 3);
BENCHMARK_MAIN();