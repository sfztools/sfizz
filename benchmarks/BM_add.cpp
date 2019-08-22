#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>
#include "../sources/SIMDHelpers.h"

class AddArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    input = std::vector<float>(state.range(0));
    output = std::vector<float>(state.range(0));
    std::generate(output.begin(), output.end(), [&]() { return dist(gen); });
    std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

  }

  std::vector<float> input;
  std::vector<float> output;
};


BENCHMARK_DEFINE_F(AddArray, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        add<float, false>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        add<float, true>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(AddArray, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        add<float, false>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_DEFINE_F(AddArray, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        add<float, true>(absl::MakeSpan(input).subspan(1), absl::MakeSpan(output).subspan(1));
    }
}

BENCHMARK_REGISTER_F(AddArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, SIMD)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, Scalar_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(AddArray, SIMD_Unaligned)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();