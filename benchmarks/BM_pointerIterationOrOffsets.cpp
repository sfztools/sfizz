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
#include <random>
#include <absl/algorithm/container.h>
#include "SIMDHelpers.h"
#include "absl/types/span.h"

constexpr int bigNumber { 2399132 };

class IterOffset : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state [[maybe_unused]]) {
    std::random_device rd { };
    std::mt19937 gen { rd() };    
    std::uniform_real_distribution<float> dist { 0.001, 1 };
    std::uniform_int_distribution<int> jumpDist { 0, 3 };
    source = std::vector<float>(bigNumber);
    result.resize(state.range(0));
    absl::c_generate(source, [&]() { return dist(gen); });
    jumps.resize(state.range(0));
    offsets.resize(state.range(0));
    absl::c_generate(jumps, [&]() { return jumpDist(gen); });
    sfz::cumsum<int>(jumps, absl::MakeSpan(offsets));
  }

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

  }

    std::vector<float> source;
    std::vector<float> result;
    std::vector<int> offsets;
    std::vector<int> jumps;
};

BENCHMARK_DEFINE_F(IterOffset, Pointers)(benchmark::State& state) {
    for (auto _ : state)
    {   
        sfz::diff<int>(offsets, absl::MakeSpan(jumps));
        auto jump = jumps.begin();
        auto in = source.begin();
        auto out = result.begin();
        while (out < result.end() && in < source.end()) {
            *out++ = *in;
            in += *jump++;
        }
    }
}

BENCHMARK_DEFINE_F(IterOffset, Offsets)(benchmark::State& state) {
    for (auto _ : state)
    {   
        auto offset = offsets.begin();
        auto out = result.begin();
        while (offset < offsets.end())
            *out++ = source[*offset++];
    }
}


// Register the function as a benchmark
BENCHMARK_REGISTER_F(IterOffset, Pointers)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK_REGISTER_F(IterOffset, Offsets)->RangeMultiplier(4)->Range((1 << 2), (1 << 12));
BENCHMARK_MAIN();