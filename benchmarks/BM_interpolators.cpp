// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "Interpolators.h"
#include "ScopedFTZ.h"
#include "absl/types/span.h"
#include <benchmark/benchmark.h>
#include <random>

class Interpolators : public benchmark::Fixture {
public:
    Interpolators()
    {
        sfz::initializeInterpolators();
    }

    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd { };
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> dist { -1.0f, 1.0f };

        const size_t numFramesIn = state.range(0);
        inputBuffer = std::vector<float>(numFramesIn + 2 * sfz::config::excessFileFrames);

        // any ratio will do, compute time will be proportional
        static constexpr float ratio = 1.234;

        const size_t numFramesOut = static_cast<size_t>(std::ceil(numFramesIn * ratio));
        input = absl::MakeSpan(inputBuffer).subspan(sfz::config::excessFileFrames, numFramesIn);
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

#define ADD_INTERPOLATOR_BENCHMARK(Type)                                \
    BENCHMARK_DEFINE_F(Interpolators, Type)(benchmark::State& state)    \
    {                                                                   \
        ScopedFTZ ftz;                                                  \
        for (auto _ : state) {                                          \
            absl::Span<float> span = absl::MakeSpan(output);            \
            doInterpolation<sfz::kInterpolator##Type>(input, span);     \
        }                                                               \
    }                                                                   \
    BENCHMARK_REGISTER_F(Interpolators, Type)                           \
        ->RangeMultiplier(4)->Range(1 << 4, 1 << 12);

ADD_INTERPOLATOR_BENCHMARK(Nearest)
ADD_INTERPOLATOR_BENCHMARK(Linear)
ADD_INTERPOLATOR_BENCHMARK(Hermite3)
ADD_INTERPOLATOR_BENCHMARK(Bspline3)
ADD_INTERPOLATOR_BENCHMARK(Sinc8)
ADD_INTERPOLATOR_BENCHMARK(Sinc12)
ADD_INTERPOLATOR_BENCHMARK(Sinc16)
ADD_INTERPOLATOR_BENCHMARK(Sinc24)
ADD_INTERPOLATOR_BENCHMARK(Sinc36)
ADD_INTERPOLATOR_BENCHMARK(Sinc48)
ADD_INTERPOLATOR_BENCHMARK(Sinc60)
ADD_INTERPOLATOR_BENCHMARK(Sinc72)
