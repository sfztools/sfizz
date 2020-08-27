// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PowerFollower.h"
#include "AudioBuffer.h"
#include "Config.h"
#include <benchmark/benchmark.h>
#include <cmath>

class PowerFollowerFixture : public benchmark::Fixture {
public:
    PowerFollowerFixture()
    {
        inputSignal_ = sfz::AudioBuffer<float>(2, numFrames);
        auto leftSignal = inputSignal_.getSpan(0);
        auto rightSignal = inputSignal_.getSpan(1);
        float phase = 0;
        for (size_t i = 0; i < numFrames; ++i) {
            constexpr float k2pi = 2.0 * M_PI;
            leftSignal[i] = std::sin(k2pi * phase);
            rightSignal[i] = std::cos(k2pi * phase);
            phase += 440.0f / sfz::config::defaultSampleRate;
            phase -= static_cast<int>(phase);
        }
    }

    void SetUp(const ::benchmark::State& state)
    {
        auto blockSize = static_cast<size_t>(state.range(0));
        follower_.setSampleRate(sfz::config::defaultSampleRate);
        follower_.setSamplesPerBlock(blockSize);
        follower_.clear();
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    static constexpr size_t numFrames = 65536;
    sfz::PowerFollower follower_;
    sfz::AudioBuffer<float> inputSignal_;
};

constexpr size_t PowerFollowerFixture::numFrames;

BENCHMARK_DEFINE_F(PowerFollowerFixture, Follower) (benchmark::State& state)
{
    sfz::AudioSpan<float> inputSignal(inputSignal_);
    for (auto _ : state) {
        auto& follower = follower_;
        auto blockSize = static_cast<size_t>(state.range(0));
        for (size_t i = 0; i < numFrames; i += blockSize)
            follower.process(inputSignal.subspan(i, blockSize));
    }
}

BENCHMARK_REGISTER_F(PowerFollowerFixture, Follower)->RangeMultiplier(2)->Range(1 << 5, 1 << 12);
