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

#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>
#include "EventEnvelopes.h"
#include "absl/types/span.h"

#include "FloatEnvelopes.cpp"

class EnvelopeFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        input = std::vector<float>(state.range(0));
        output = std::vector<float>(state.range(0));
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
        sfz::cumsum<float, false>(input, absl::MakeSpan(input));
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]])
    {

    }
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 1, 30 };
    std::vector<float> input;
    std::vector<float> output;
};


BENCHMARK_DEFINE_F(EnvelopeFixture, Linear)(benchmark::State& state) {
    sfz::LinearEnvelope<float> envelope;
    for (auto _ : state)
    {
        envelope.registerEvent(static_cast<int>(state.range(0) - 1), dist(gen));
        envelope.getBlock(absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, LinearQuantized)(benchmark::State& state) {
    sfz::LinearEnvelope<float> envelope;
    for (auto _ : state)
    {
        envelope.registerEvent(static_cast<int>(state.range(0) - 1), dist(gen));
        envelope.getQuantizedBlock(absl::MakeSpan(output), 0.5);
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, Multiplicative)(benchmark::State& state) {
    sfz::MultiplicativeEnvelope<float> envelope;
    for (auto _ : state)
    {
        envelope.registerEvent(static_cast<int>(state.range(0) - 1), dist(gen));
        envelope.getBlock(absl::MakeSpan(output));
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, MultiplicativeQuantized)(benchmark::State& state) {
    sfz::MultiplicativeEnvelope<float> envelope;
    for (auto _ : state)
    {
        envelope.registerEvent(static_cast<int>(state.range(0) - 1), dist(gen));
        envelope.getQuantizedBlock(absl::MakeSpan(output), 1.5);
    }
}


BENCHMARK_REGISTER_F(EnvelopeFixture, Linear)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, LinearQuantized)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, Multiplicative)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, MultiplicativeQuantized)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
