#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <numeric>
#include <absl/algorithm/container.h>
#include "../sources/SIMDHelpers.h"

// In this one we have an array of indices

constexpr int loopEnd { 1076 };
constexpr float maxJump { 4 };

class SaturatingFixture : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, maxJump };
    indices = std::vector<int>(state.range(0));
    leftCoeffs = std::vector<float>(state.range(0));
    rightCoeffs = std::vector<float>(state.range(0));
    jumps = std::vector<float>(state.range(0));
    absl::c_generate(jumps, [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

  }

    std::vector<int> indices;
    std::vector<float> leftCoeffs;
    std::vector<float> rightCoeffs;
    std::vector<float> jumps;
};


BENCHMARK_DEFINE_F(SaturatingFixture, Scalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        saturatingSFZIndex<float, false>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 2.5f, loopEnd);
    }
}

BENCHMARK_DEFINE_F(SaturatingFixture, SIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        saturatingSFZIndex<float, true>(jumps, absl::MakeSpan(leftCoeffs), absl::MakeSpan(rightCoeffs), absl::MakeSpan(indices), 2.5f, loopEnd);
    }
}

BENCHMARK_DEFINE_F(SaturatingFixture, Scalar_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        saturatingSFZIndex<float, false>(absl::MakeSpan(jumps).subspan(1), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1), absl::MakeSpan(indices).subspan(3), 2.5f, loopEnd);
    }
}

BENCHMARK_DEFINE_F(SaturatingFixture, SIMD_Unaligned)(benchmark::State& state) {
    for (auto _ : state)
    {
        saturatingSFZIndex<float, true>(absl::MakeSpan(jumps).subspan(1), absl::MakeSpan(leftCoeffs).subspan(2), absl::MakeSpan(rightCoeffs).subspan(1), absl::MakeSpan(indices).subspan(3), 2.5f, loopEnd);
    }
}


// Register the function as a benchmark
BENCHMARK_REGISTER_F(SaturatingFixture, Scalar)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(SaturatingFixture, SIMD)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(SaturatingFixture, Scalar_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_REGISTER_F(SaturatingFixture, SIMD_Unaligned)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();