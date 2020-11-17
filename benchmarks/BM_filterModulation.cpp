// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "OnePoleFilter.h"
#include "SfzFilter.h"
#include "SfzHelpers.h"
#include "ScopedFTZ.h"
#include "SfzHelpers.h"
#include <benchmark/benchmark.h>
#include <random>
#include <numeric>
#include <vector>
#include <cmath>
#include <iostream>

constexpr int blockSize { 1024 };
constexpr float sampleRate { 48000.0f };

class FilterFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) {
        input = std::vector<float>(blockSize);
        output = std::vector<float>(blockSize);
        cutoff = std::vector<float>(blockSize);
        q = std::vector<float>(blockSize);
        pksh = std::vector<float>(blockSize);
        sfz::linearRamp<float>(absl::MakeSpan(cutoff), 500, 1.0f);
        sfz::linearRamp<float>(absl::MakeSpan(q), 0.0f, 0.001f);
        sfz::linearRamp<float>(absl::MakeSpan(pksh), 0.0f, 0.001f);
        std::generate(input.begin(), input.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& /* state */) {

    }
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { 0, 0.5 };
    std::vector<float> cutoff;
    std::vector<float> q;
    std::vector<float> pksh;
    std::vector<float> input;
    std::vector<float> output;
};

BENCHMARK_DEFINE_F(FilterFixture, OnePole_VA)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::OnePoleFilter<float> filter;
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffPtr = cutoff.data();
        auto inputPtr = input.data();
        auto outputPtr = output.data();
        const auto sentinel = cutoff.data() + blockSize;
        while (cutoffPtr < sentinel)
        {
            const auto gain = sfz::vaGain(*cutoffPtr, sampleRate);
            filter.setGain(gain);
            filter.processLowpass({ inputPtr, step }, { outputPtr, step } );
            cutoffPtr += step;
            inputPtr += step;
            outputPtr += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, OnePole_Faust)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::Filter filter;
    filter.init(sampleRate);
    filter.setType(sfz::FilterType::kFilterLpf1p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffPtr = cutoff.data();
        auto inputPtr = input.data();
        auto outputPtr = output.data();
        const auto sentinel = cutoff.data() + blockSize;
        while (cutoffPtr < sentinel)
        {
            filter.process(&inputPtr, &outputPtr, *cutoffPtr, 0.0, 0.0, step);
            cutoffPtr += step;
            inputPtr += step;
            outputPtr += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, TwoPole_Faust)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::Filter filter;
    filter.init(sampleRate);
    filter.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffPtr = cutoff.data();
        auto qIterator = q.begin();
        auto inputPtr = input.data();
        auto outputPtr = output.data();
        const auto sentinel = cutoff.data() + blockSize;
        while (cutoffPtr < sentinel)
        {
            filter.process(&inputPtr, &outputPtr, *cutoffPtr, *qIterator, 0.0, step);
            qIterator += step;
            cutoffPtr += step;
            inputPtr += step;
            outputPtr += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, TwoPoleShelf_Faust)(benchmark::State& state) {
    ScopedFTZ ftz;
    sfz::Filter filter;
    filter.init(sampleRate);
    filter.setType(sfz::FilterType::kFilterLsh);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffPtr = cutoff.data();
        auto qIterator = q.begin();
        auto pkshIterator = pksh.begin();
        auto inputPtr = input.data();
        auto outputPtr = output.data();
        const auto sentinel = cutoff.data() + blockSize;
        while (cutoffPtr < sentinel)
        {
            filter.process(&inputPtr, &outputPtr, *cutoffPtr, *qIterator, *pkshIterator, step);
            qIterator += step;
            cutoffPtr += step;
            pkshIterator += step;
            inputPtr += step;
            outputPtr += step;
        }
    }
}

BENCHMARK_REGISTER_F(FilterFixture, OnePole_VA)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, OnePole_Faust)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, TwoPole_Faust)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, TwoPoleShelf_Faust)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_MAIN();

