// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Interpolators.h"
#include "ScopedFTZ.h"
#include "absl/types/span.h"
#include <benchmark/benchmark.h>
#include <random>

class Interpolators : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd { };
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { -1.0f, 1.0f };

        const size_t numFramesIn = state.range(0);
        inputBuffer = std::vector<float>(numFramesIn + excessFrames);

        // any ratio will do, compute time will be proportional
        static constexpr float ratio = 1.234;

        const size_t numFramesOut = static_cast<size_t>(std::ceil(numFramesIn * ratio));
        input = absl::MakeSpan(inputBuffer.data(), numFramesIn);
        output = std::vector<float>(numFramesOut);
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    std::vector<float> inputBuffer;
    absl::Span<float> input;
    std::vector<float> output;

    enum { excessFrames = 8 };
};

template <sfz::InterpolatorModel M>
static void doInterpolation(absl::Span<const float> input, absl::Span<float> output)
{
    const float kOutToIn = static_cast<float>(input.size()) / output.size();

    for (size_t iOut = 0; iOut < output.size(); ++iOut) {
        float posIn = iOut * kOutToIn;
        unsigned dec = static_cast<int>(posIn);
        float frac = posIn - dec;
        output[iOut] = sfz::interpolate<M>(&input[dec], frac);
    }
}

BENCHMARK_DEFINE_F(Interpolators, Linear)(benchmark::State& state)
{
    ScopedFTZ ftz;

    for (auto _ : state) {
        doInterpolation<sfz::kInterpolatorLinear>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(Interpolators, Hermite3)(benchmark::State& state)
{
    ScopedFTZ ftz;

    for (auto _ : state) {
        doInterpolation<sfz::kInterpolatorHermite3>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(Interpolators, Bspline3)(benchmark::State& state)
{
    ScopedFTZ ftz;

    for (auto _ : state) {
        doInterpolation<sfz::kInterpolatorBspline3>(input, absl::MakeSpan(output));
    }
}

BENCHMARK_REGISTER_F(Interpolators, Linear)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(Interpolators, Hermite3)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(Interpolators, Bspline3)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
