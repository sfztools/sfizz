// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "Buffer.h"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <numeric>
#include <absl/types/span.h>

static void Scalar(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);

  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void SSE(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);

  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void Scalar_Unaligned(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(
        absl::MakeSpan(input).subspan(2),
        absl::MakeSpan(outputLeft),
        absl::MakeSpan(outputRight)
    );
  }
}

static void SSE_Unaligned(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(
        absl::MakeSpan(input).subspan(2),
        absl::MakeSpan(outputLeft),
        absl::MakeSpan(outputRight)
    );
  }
}

static void Scalar_Unaligned_2(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, false);
    sfz::readInterleaved(
        absl::MakeSpan(input).subspan(2),
        absl::MakeSpan(outputLeft).subspan(1),
        absl::MakeSpan(outputRight).subspan(3)
    );
  }
}

static void SSE_Unaligned_2(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::readInterleaved, true);
    sfz::readInterleaved(
        absl::MakeSpan(input).subspan(2),
        absl::MakeSpan(outputLeft).subspan(1),
        absl::MakeSpan(outputRight).subspan(3)
    );
  }
}

BENCHMARK(Scalar)->Range((8<<10), (8<<20));
BENCHMARK(SSE)->Range((8<<10), (8<<20));
BENCHMARK(Scalar_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK(SSE_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK(Scalar_Unaligned_2)->Range((8<<10), (8<<20));
BENCHMARK(SSE_Unaligned_2)->Range((8<<10), (8<<20));
BENCHMARK_MAIN();
