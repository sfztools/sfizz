// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz
#include "SIMDHelpers.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>
#include "ModifierHelpers.h"
#include "absl/types/span.h"

class EnvelopeFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        input = std::vector<float>(state.range(0));
        output = std::vector<float>(state.range(0));
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
        sfz::cumsum<float>(input, absl::MakeSpan(input));
    }

    void TearDown(const ::benchmark::State& /* state */)
    {

    }
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 2, 30 };
    std::vector<float> input;
    std::vector<float> output;
};


BENCHMARK_DEFINE_F(EnvelopeFixture, Linear)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::EventVector events {
            { 0, 0.0f },
            { static_cast<int>(state.range(0) - 1), dist(gen) }
        };
        sfz::linearEnvelope(events, absl::MakeSpan(output), [](float x) { return x; });
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, LinearNoEvent) (benchmark::State& state) {
    for (auto _ : state) {
        sfz::EventVector events {
            { 0, dist(gen) }
        };
        sfz::linearEnvelope(events, absl::MakeSpan(output), [](float x) { return x; });
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, LinearQuantized)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::EventVector events {
            { 0, 0.0f },
            { static_cast<int>(state.range(0) - 1), dist(gen) }
        };
        sfz::linearEnvelope(
            events, absl::MakeSpan(output), [](float x) { return x; }, 0.5);
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, Multiplicative)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::EventVector events {
            { 0, 1.0f },
            { static_cast<int>(state.range(0) - 1), dist(gen) }
        };
        sfz::multiplicativeEnvelope(events, absl::MakeSpan(output), [](float x) { return x; });
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, MultiplicativeNoEvent) (benchmark::State& state) {
    for (auto _ : state) {
        sfz::EventVector events {
            { 0, dist(gen) }
        };
        sfz::multiplicativeEnvelope(events, absl::MakeSpan(output), [](float x) { return x; });
    }
}

BENCHMARK_DEFINE_F(EnvelopeFixture, MultiplicativeQuantized)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::EventVector events {
            { 0, 1.0f },
            { static_cast<int>(state.range(0) - 1), dist(gen) }
        };
        sfz::multiplicativeEnvelope(
            events, absl::MakeSpan(output), [](float x) { return x; }, 2.0f);
    }
}


BENCHMARK_REGISTER_F(EnvelopeFixture, Linear)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, LinearNoEvent)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, LinearQuantized)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, Multiplicative)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, MultiplicativeNoEvent)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(EnvelopeFixture, MultiplicativeQuantized)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
