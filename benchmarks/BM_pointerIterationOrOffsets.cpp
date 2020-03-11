// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <random>
#include <absl/algorithm/container.h>
#include "absl/types/span.h"

constexpr int bigNumber { 2399132 };

class IterOffset : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0.001f, 1.0f };
    std::uniform_int_distribution<int> jumpDist { 0, 3 };
    source = std::vector<float>(bigNumber);
    result.resize(state.range(0));
    absl::c_generate(source, [&]() { return dist(gen); });
    jumps.resize(state.range(0));
    offsets.resize(state.range(0));
    absl::c_generate(jumps, [&]() { return jumpDist(gen); });
    sfz::cumsum<int>(jumps, absl::MakeSpan(offsets));
  }

  void TearDown(const ::benchmark::State& /* state */) {

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
