// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ScopedFTZ.h"
#include "MathHelpers.h"
#include "SIMDConfig.h"
#include "utility/Macros.h"
#include "effects/impl/ResonantArray.h"
#include "effects/impl/ResonantArraySSE.h"
#include "effects/impl/ResonantArrayAVX.h"
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

class StringResonator : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) {
        std::random_device rd { };
        std::mt19937 gen { rd() };

        numStrings = state.range(0);
        pitches.resize(numStrings);
        bandwidths.resize(numStrings);
        feedbacks.resize(numStrings);
        gains.resize(numStrings);

        std::generate(pitches.begin(), pitches.end(), [&]() {
            std::uniform_int_distribution<> dist {0, 127};
            return midiNoteFrequency(dist(gen));
        });

        std::fill(bandwidths.begin(), bandwidths.end(), 1.0f);
        std::fill(feedbacks.begin(), feedbacks.end(), std::exp(-6.91 / (50e-3 * sampleRate)));
        std::fill(gains.begin(), gains.end(), 1e-3f);

        input.resize(numFrames);
        output.resize(numFrames);

        std::generate(input.begin(), input.end(), [&]() {
            std::uniform_real_distribution<> dist {-1, 1};
            return dist(gen);
        });
    }

    void TearDown(const ::benchmark::State& state) {
        UNUSED(state);
    }

    static constexpr float sampleRate = 44100;
    static constexpr unsigned numFrames = sampleRate * 1.0;
    std::vector<float> input;
    std::vector<float> output;
    unsigned numStrings = 0;
    std::vector<float> pitches;
    std::vector<float> bandwidths;
    std::vector<float> feedbacks;
    std::vector<float> gains;
};

BENCHMARK_DEFINE_F(StringResonator, StringResonator_Scalar)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::fx::ResonantArrayScalar resonator;
    resonator.setup(sampleRate, numStrings, pitches.data(), bandwidths.data(), feedbacks.data(), gains.data());
    resonator.setSamplesPerBlock(numFrames);
    for (auto _ : state)
    {
        resonator.process(input.data(), output.data(), numFrames);
    }
}

#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
BENCHMARK_DEFINE_F(StringResonator, StringResonator_SSE)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::fx::ResonantArraySSE resonator;
    resonator.setup(sampleRate, numStrings, pitches.data(), bandwidths.data(), feedbacks.data(), gains.data());
    resonator.setSamplesPerBlock(numFrames);
    for (auto _ : state)
    {
        resonator.process(input.data(), output.data(), numFrames);
    }
}

BENCHMARK_DEFINE_F(StringResonator, StringResonator_AVX)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::fx::ResonantArrayAVX resonator;
    resonator.setup(sampleRate, numStrings, pitches.data(), bandwidths.data(), feedbacks.data(), gains.data());
    resonator.setSamplesPerBlock(numFrames);
    for (auto _ : state)
    {
        resonator.process(input.data(), output.data(), numFrames);
    }
}
#endif

BENCHMARK_REGISTER_F(StringResonator, StringResonator_Scalar)->RangeMultiplier(4)->Range(1, 128);
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
BENCHMARK_REGISTER_F(StringResonator, StringResonator_SSE)->RangeMultiplier(4)->Range(1, 128);
BENCHMARK_REGISTER_F(StringResonator, StringResonator_AVX)->RangeMultiplier(4)->Range(1, 128);
#endif
BENCHMARK_MAIN();
