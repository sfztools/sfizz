// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
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
    sfz::readInterleaved<float, false>(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void SSE(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);

  for (auto _ : state) {
    sfz::readInterleaved<float, true>(input, absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void Scalar_Unaligned(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::readInterleaved<float, false>(absl::MakeSpan(input).subspan(2), absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void SSE_Unaligned(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::readInterleaved<float, true>(absl::MakeSpan(input).subspan(2), absl::MakeSpan(outputLeft), absl::MakeSpan(outputRight));
  }
}

static void Scalar_Unaligned_2(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::readInterleaved<float, false>(absl::MakeSpan(input).subspan(2), absl::MakeSpan(outputLeft).subspan(1), absl::MakeSpan(outputRight).subspan(3));
  }
}

static void SSE_Unaligned_2(benchmark::State& state) {
  sfz::Buffer<float> input (state.range(0) * 2);
  sfz::Buffer<float> outputLeft (state.range(0));
  sfz::Buffer<float> outputRight (state.range(0));
  std::iota(input.begin(), input.end(), 1.0f);
  for (auto _ : state) {
    sfz::readInterleaved<float, true>(absl::MakeSpan(input).subspan(2), absl::MakeSpan(outputLeft).subspan(1), absl::MakeSpan(outputRight).subspan(3));
  }
}

BENCHMARK(Scalar)->Range((8<<10), (8<<20));
BENCHMARK(SSE)->Range((8<<10), (8<<20));
BENCHMARK(Scalar_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK(SSE_Unaligned)->Range((8<<10), (8<<20));
BENCHMARK(Scalar_Unaligned_2)->Range((8<<10), (8<<20));
BENCHMARK(SSE_Unaligned_2)->Range((8<<10), (8<<20));
BENCHMARK_MAIN();
