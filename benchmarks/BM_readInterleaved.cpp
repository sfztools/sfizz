#include <benchmark/benchmark.h>
#include "../sources/SIMDHelpers.h"
#include "../sources/Buffer.h"
#include <algorithm>
#include <numeric>
#include <absl/types/span.h>

static void Scalar(benchmark::State& state) {
  Buffer<float> input (state.range(0) * 2);
  Buffer<float> outputLeft (state.range(0));
  Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);

  for (auto _ : state) {
    readInterleaved<float, false>(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void SSE(benchmark::State& state) {
  Buffer<float> input (state.range(0) * 2);
  Buffer<float> outputLeft (state.range(0));
  Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);

  for (auto _ : state) {
    readInterleaved<float, true>(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void Scalar_Unaligned(benchmark::State& state) {
  Buffer<float> input (state.range(0) * 2);
  Buffer<float> outputLeft (state.range(0));
  Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    readInterleaved<float, false>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(outputLeft).subspan(1), absl::MakeSpan(outputRight).subspan(1));
  }
}

static void SSE_Unaligned(benchmark::State& state) {
  Buffer<float> input (state.range(0) * 2);
  Buffer<float> outputLeft (state.range(0));
  Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    readInterleaved<float, true>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(outputLeft).subspan(1), absl::MakeSpan(outputRight).subspan(1));
  }
}

BENCHMARK(Scalar)->Range((8<<10), (8<<20));
BENCHMARK(SSE)->Range((8<<10), (8<<20));
BENCHMARK(Scalar_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK(SSE_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK_MAIN();