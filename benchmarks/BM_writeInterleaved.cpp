// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <benchmark/benchmark.h>
#include "../sfizz/SIMDHelpers.h"
#include "../sfizz/Buffer.h"
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