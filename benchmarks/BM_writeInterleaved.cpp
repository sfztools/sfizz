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

static void Interleaved_Write(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);

  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(inputLeft, inputRight, absl::MakeSpan(output));
  }
}

static void Interleaved_Write_SSE(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(inputLeft, inputRight, absl::MakeSpan(output));
  }
}

static void Unaligned_Interleaved_Write(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(
        absl::MakeSpan(inputLeft).subspan(1),
        absl::MakeSpan(inputRight).subspan(1),
        absl::MakeSpan(output).subspan(2)
    );
  }
}

static void Unaligned_Interleaved_Write_SSE(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(
        absl::MakeSpan(inputLeft).subspan(1),
        absl::MakeSpan(inputRight).subspan(1),
        absl::MakeSpan(output).subspan(2)
    );
  }
}

static void Unaligned_Interleaved_Write_2(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, false);
    sfz::writeInterleaved(
        absl::MakeSpan(inputLeft),
        absl::MakeSpan(inputRight).subspan(1),
        absl::MakeSpan(output).subspan(2)
    );
  }
}

static void Unaligned_Interleaved_Write_SSE_2(benchmark::State& state) {
  sfz::Buffer<float> inputLeft (state.range(0));
  sfz::Buffer<float> inputRight (state.range(0));
  sfz::Buffer<float> output (state.range(0) * 2);
  std::iota(inputLeft.begin(), inputLeft.end(), 1.0f);
  std::iota(inputRight.begin(), inputRight.end(), 1.0f);
  for (auto _ : state) {
    sfz::setSIMDOpStatus<float>(sfz::SIMDOps::writeInterleaved, true);
    sfz::writeInterleaved(
        absl::MakeSpan(inputLeft),
        absl::MakeSpan(inputRight).subspan(1),
        absl::MakeSpan(output).subspan(2)
    );
  }
}

BENCHMARK(Interleaved_Write)->Range((8<<10), (8<<20));
BENCHMARK(Interleaved_Write_SSE)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_SSE)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_2)->Range((8<<10), (8<<20));
BENCHMARK(Unaligned_Interleaved_Write_SSE_2)->Range((8<<10), (8<<20));
BENCHMARK_MAIN();
