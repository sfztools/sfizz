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
#include <random>
#include <numeric>

static void Dummy(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    auto fillValue = dist(gen);
    benchmark::DoNotOptimize(fillValue);
  }
}

static void FillScalar(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, false>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillScalar_unaligned(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, false>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

static void FillSIMD(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, true>(absl::MakeSpan(buffer), dist(gen));
  }
}

static void FillSIMD_unaligned(benchmark::State& state) {
  sfz::Buffer<float> buffer (state.range(0));
  std::random_device rd { };
  std::mt19937 gen { rd() };
  std::uniform_real_distribution<float> dist { 1, 2 };
  for (auto _ : state) {
    sfz::fill<float, true>(absl::MakeSpan(buffer).subspan(1), dist(gen));
  }
}

BENCHMARK(Dummy)->Range((2<<6), (2<<16));
BENCHMARK(FillScalar)->Range((2<<6), (2<<16));
BENCHMARK(FillSIMD)->Range((2<<6), (2<<16));
BENCHMARK(FillScalar_unaligned)->Range((2<<6), (2<<16));
BENCHMARK(FillSIMD_unaligned)->Range((2<<6), (2<<16));
BENCHMARK_MAIN();