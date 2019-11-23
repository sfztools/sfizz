// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <benchmark/benchmark.h>
#include "ADSREnvelope.h"
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
        for(int offset = 0; offset < envelopeSize; offset += state.range(0))
            for (auto& out: output)
                out = envelope.getNextValue();
        benchmark::DoNotOptimize(output);
    }
    state.counters["Per block"] = benchmark::Counter(envelopeSize / state.range(0), benchmark::Counter::kIsRate);
}

static void Block(benchmark::State& state) {
    std::vector<float> output(state.range(0));
    sfz::ADSREnvelope<float> envelope;
    for (auto _ : state) {
        envelope.reset(attack, release, 1.0, fixedAmount, decay, fixedAmount);
        envelope.startRelease(releaseTime);
        for(int offset = 0; offset < envelopeSize; offset += state.range(0))
            envelope.getBlock(absl::MakeSpan(output));
        benchmark::DoNotOptimize(output);
    }

    state.counters["Per block"] = benchmark::Counter(envelopeSize / state.range(0), benchmark::Counter::kIsRate);
}

BENCHMARK(Point)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK(Block)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK_MAIN();