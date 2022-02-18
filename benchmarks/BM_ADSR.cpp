// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "ADSREnvelope.h"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <random>
#include <numeric>

constexpr int fixedAmount{12};
constexpr float sampleRate{100.0f};
constexpr int envelopeSize{2 << 16};
constexpr float attack = static_cast<float>(static_cast<int>(envelopeSize / 4) - fixedAmount) / sampleRate;
constexpr float decay = attack;
constexpr float release = attack;
constexpr float releaseTime = envelopeSize - static_cast<int>(envelopeSize / 4);

class EnvelopeFixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State &state)
    {
        region.amplitudeEG.attack = attack;
        region.amplitudeEG.release = release;
        region.amplitudeEG.decay = decay;
        output.resize(state.range(0));
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    sfz::MidiState midiState;
    sfz::Region region{0};
    sfz::ADSREnvelope envelope;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(EnvelopeFixture, Block)(benchmark::State& state)
{
    for (auto _ : state) {
        envelope.reset(region.amplitudeEG, region, midiState, 0, 0, sampleRate);
        envelope.startRelease(releaseTime);
        for (int offset = 0; offset < envelopeSize; offset += static_cast<int>(state.range(0)))
            envelope.getBlock(absl::MakeSpan(output));
        benchmark::DoNotOptimize(output);
    }

    state.counters["Blocks"] = benchmark::Counter(envelopeSize / static_cast<double>(state.range(0)), benchmark::Counter::kIsIterationInvariantRate);
}

BENCHMARK_REGISTER_F(EnvelopeFixture, Block)->RangeMultiplier(2)->Range((2 << 6), (2 << 11));
BENCHMARK_MAIN();
