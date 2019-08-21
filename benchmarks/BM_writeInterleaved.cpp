#include <benchmark/benchmark.h>
#include "../sources/SIMDHelpers.h"
#include "../sources/Buffer.h"
#include <algorithm>
#include <numeric>
#include <absl/types/span.h>

static void Interleaved_Write(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);

  for (auto _ : state) {
    writeInterleaved<float, false>(inputLeft, inputRight, absl::MakeSpan(output));
  }
}

static void Interleaved_Write_SSE(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    writeInterleaved<float, true>(inputLeft, inputRight, absl::MakeSpan(output));
    benchmark::DoNotOptimize(output);
  }
}

static void Unaligned_Interleaved_Write(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    writeInterleaved<float, false>(absl::MakeSpan(inputLeft).subspan(1) , absl::MakeSpan(inputRight).subspan(1), absl::MakeSpan(output).subspan(2));
    benchmark::DoNotOptimize(output);
  }
}

static void Unaligned_Interleaved_Write_SSE(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    writeInterleaved<float, true>(absl::MakeSpan(inputLeft).subspan(1) , absl::MakeSpan(inputRight).subspan(1), absl::MakeSpan(output).subspan(2));
    benchmark::DoNotOptimize(output);
  }
}

static void Unaligned_Interleaved_Write_2(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    writeInterleaved<float, false>(absl::MakeSpan(inputLeft) , absl::MakeSpan(inputRight).subspan(1), absl::MakeSpan(output).subspan(2));
    benchmark::DoNotOptimize(output);
  }
}

static void Unaligned_Interleaved_Write_SSE_2(benchmark::State& state) {
  Buffer<float> inputLeft (state.range(0));
  Buffer<float> inputRight (state.range(0));
  Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    writeInterleaved<float, true>(absl::MakeSpan(inputLeft) , absl::MakeSpan(inputRight).subspan(1), absl::MakeSpan(output).subspan(2));
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK(Interleaved_Write)->Range((8<<10), (8<<20));
BENCHMARK(Interleaved_Write_SSE)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_SSE)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_2)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_SSE_2)->Range((8<<10), (8<<20));
BENCHMARK_MAIN();