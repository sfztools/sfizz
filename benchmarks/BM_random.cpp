// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "MathHelpers.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <random>

class RandomFill : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    output = std::vector<float>(state.range(0));
  }

  void TearDown(const ::benchmark::State& state) {
      UNUSED(state);
  }

  std::vector<float> output;
};

BENCHMARK_DEFINE_F(RandomFill, StdRandom)(benchmark::State& state) {
    std::minstd_rand prng;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = dist(prng);
    }
}

BENCHMARK_DEFINE_F(RandomFill, FastRandom)(benchmark::State& state) {
    fast_rand prng;
    fast_real_distribution<float> dist(0.0f, 1.0f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = dist(prng);
    }
}
BENCHMARK_DEFINE_F(RandomFill, StdRandomBipolar)(benchmark::State& state) {
    std::minstd_rand prng;
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = dist(prng);
    }
}

BENCHMARK_DEFINE_F(RandomFill, FastRandomBipolar)(benchmark::State& state) {
    fast_rand prng;
    fast_real_distribution<float> dist(-1.0f, 1.0f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = dist(prng);
    }
}

BENCHMARK_DEFINE_F(RandomFill, StdNormal)(benchmark::State& state) {
    std::minstd_rand prng;
    std::normal_distribution<float> dist(0.0f, 0.25f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = dist(prng);
    }
}

BENCHMARK_DEFINE_F(RandomFill, FastNormal)(benchmark::State& state) {
    fast_gaussian_generator<float, 4> generator(0.0f, 0.25f);

    for (auto _ : state)
    {
        for (float &out : output)
            out = generator();
    }
}

BENCHMARK_REGISTER_F(RandomFill, StdRandom)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(RandomFill, StdRandomBipolar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(RandomFill, FastRandom)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(RandomFill, FastRandomBipolar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(RandomFill, StdNormal)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(RandomFill, FastNormal)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
