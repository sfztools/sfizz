// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "ADSREnvelope.h"
#include "BM.h"
#include <algorithm>
#include <random>
#include <numeric>

constexpr int fixedAmount { 12 };
constexpr int envelopeSize { 2 << 16 };
constexpr int attack = static_cast<int>(envelopeSize / 4) - fixedAmount;
constexpr int decay = attack;
constexpr int release = attack;
constexpr int releaseTime = envelopeSize - static_cast<int>(envelopeSize / 4);

// Explicitely instantiate class
#include "ADSREnvelope.cpp"
template class sfz::ADSREnvelope<float>;

static void Point(benchmark::State& state) {
    std::vector<float> output(state.range(0));
    sfz::ADSREnvelope<float> envelope;
    for (auto _ : state) {
        envelope.reset(attack, release, 1.0, fixedAmount, decay, fixedAmount);
        envelope.startRelease(releaseTime);
        for(int offset = 0; offset < envelopeSize; offset += static_cast<int>(state.range(0)))
            for (auto& out: output)
                out = envelope.getNextValue();
        benchmark::DoNotOptimize(output);
    }
    state.counters["Per block"] = benchmark::Counter(envelopeSize / static_cast<double>(state.range(0)), benchmark::Counter::kIsRate);
}

static void Block(benchmark::State& state) {
    std::vector<float> output(state.range(0));
    sfz::ADSREnvelope<float> envelope;
    for (auto _ : state) {
        envelope.reset(attack, release, 1.0, fixedAmount, decay, fixedAmount);
        envelope.startRelease(releaseTime);
        for(int offset = 0; offset < envelopeSize; offset += static_cast<int>(state.range(0)))
            envelope.getBlock(absl::MakeSpan(output));
        benchmark::DoNotOptimize(output);
    }

    state.counters["Per block"] = benchmark::Counter(envelopeSize / static_cast<double>(state.range(0)), benchmark::Counter::kIsRate);
}

BENCHMARK(Point)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK(Block)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK_MAIN();
