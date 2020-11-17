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

        //
        refFollower_.init(sfz::config::defaultSampleRate);
        refFollower_.clear();
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    static constexpr size_t numFrames = 65536;
    sfz::PowerFollower follower_;
    sfz::AudioBuffer<float> inputSignal_;

    //
    struct ReferenceFollower {
        /*
          import("stdfaust.lib");
          process = (_, _) : + : an.amp_follower_ud(att, rel) with { att = 5e-3; rel = 200e-3; };
        */

        void init(float sampleRate)
        {
            fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(sampleRate)));
            fConst1 = std::exp((0.0f - (200.0f / fConst0)));
            fConst2 = (1.0f - fConst1);
            fConst3 = std::exp((0.0f - (5.0f / fConst0)));
            fConst4 = (1.0f - fConst3);
        }
        void clear()
        {
            for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
                fRec1[l0] = 0.0f;
            }
            for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
                fRec0[l1] = 0.0f;
            }
        }
        float process(float input0, float input1)
        {
            float fTemp0 = std::fabs((float(input0) + float(input1)));
            fRec1[0] = std::max<float>(fTemp0, ((fConst3 * fRec1[1]) + (fConst4 * fTemp0)));
            fRec0[0] = ((fConst1 * fRec0[1]) + (fConst2 * fRec1[0]));
            float output = fRec0[0];
            fRec1[1] = fRec1[0];
            fRec0[1] = fRec0[0];
            return output;
        }

        float fConst0;
        float fConst1;
        float fConst2;
        float fConst3;
        float fConst4;
        float fRec1[2];
        float fRec0[2];
    };
    ReferenceFollower refFollower_;
};

constexpr size_t PowerFollowerFixture::numFrames;

BENCHMARK_DEFINE_F(PowerFollowerFixture, ReferenceFollower) (benchmark::State& state)
{
    sfz::AudioSpan<float> inputSignal(inputSignal_);
    auto input0 = inputSignal.getConstSpan(0);
    auto input1 = inputSignal.getConstSpan(1);

    for (auto _ : state) {
        auto& follower = refFollower_;
        float output = 0;
        for (size_t i = 0, n = inputSignal.getNumFrames(); i < n; ++i)
            output = follower.process(input0[i], input1[i]);
        benchmark::DoNotOptimize(output);
    }
}

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

BENCHMARK_REGISTER_F(PowerFollowerFixture, ReferenceFollower)->Range(1, 1);
BENCHMARK_REGISTER_F(PowerFollowerFixture, Follower)->RangeMultiplier(2)->Range(1 << 5, 1 << 12);
