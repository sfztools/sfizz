// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "OnePoleFilter.h"
#include "SfzFilter.h"
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
        inputLeft = std::vector<float>(blockSize);
        inputRight = std::vector<float>(blockSize);
        outputLeft = std::vector<float>(blockSize);
        outputRight = std::vector<float>(blockSize);
        cutoff = std::vector<float>(blockSize);
        q = std::vector<float>(blockSize);
        pksh = std::vector<float>(blockSize);
        sfz::linearRamp<float>(absl::MakeSpan(cutoff), 500, 1.0f);
        sfz::linearRamp<float>(absl::MakeSpan(q), 0.0f, 0.001f);
        sfz::linearRamp<float>(absl::MakeSpan(pksh), 0.0f, 0.001f);
        std::generate(inputLeft.begin(), inputLeft.end(), [&]() { return dist(gen); });
        std::generate(inputRight.begin(), inputRight.end(), [&]() { return dist(gen); });
    }

    void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

    }
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::normal_distribution<float> dist { 0, 0.5 };
    std::vector<float> cutoff;
    std::vector<float> q;
    std::vector<float> pksh;
    std::vector<float> inputLeft;
    std::vector<float> inputRight;
    std::vector<float> outputLeft;
    std::vector<float> outputRight;
};

BENCHMARK_DEFINE_F(FilterFixture, OnePole_MonoOnce)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf1p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto inLIterator = inputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process( &inLIterator.base(), &outLIterator.base(), *cutoffIterator, 0.0, 0.0, step);
            cutoffIterator += step;
            inLIterator += step;
            outLIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, OnePole_MonoTwice)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf1p);
    filterRight.setType(sfz::FilterType::kFilterLpf1p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process( &inLIterator.base(), &outLIterator.base(), *cutoffIterator, 0.0, 0.0, step);
            filterRight.process(&inRIterator.base(), &outRIterator.base(), *cutoffIterator, 0.0, 0.0, step);
            cutoffIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, OnePole_Stereo)(benchmark::State& state) {
    sfz::Filter filter;
    filter.setChannels(2);
    filter.setType(sfz::FilterType::kFilterLpf1p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            float * inputs[2] = { inLIterator.base(), inRIterator.base() };
            float * outputs[2] = { outLIterator.base(), outRIterator.base() };
            filter.process(inputs, outputs, *cutoffIterator, 0.0, 0.0, step);
            cutoffIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, TwoPole_MonoOnce)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto inLIterator = inputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process(&inLIterator.base(), &outLIterator.base(), *cutoffIterator, *qIterator, 0.0, step);
            cutoffIterator += step;
            qIterator += step;
            inLIterator += step;
            outLIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, TwoPole_MonoTwice)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf2p);
    filterRight.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process( &inLIterator.base(), &outLIterator.base(), *cutoffIterator, *qIterator, 0.0, step);
            filterRight.process(&inRIterator.base(), &outRIterator.base(), *cutoffIterator, *qIterator, 0.0, step);
            cutoffIterator += step;
            qIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, TwoPole_Stereo)(benchmark::State& state) {
    sfz::Filter filter;
    filter.setChannels(2);
    filter.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            float * inputs[2] = { inLIterator.base(), inRIterator.base() };
            float * outputs[2] = { outLIterator.base(), outRIterator.base() };
            filter.process(inputs, outputs, *cutoffIterator, *qIterator, 0.0, step);
            cutoffIterator += step;
            qIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, Shelf_MonoOnce)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto pkshIterator = pksh.begin();
        auto inLIterator = inputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process(&inLIterator.base(), &outLIterator.base(), *cutoffIterator, *qIterator, *pkshIterator, step);
            cutoffIterator += step;
            qIterator += step;
            pkshIterator += step;
            inLIterator += step;
            outLIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, Shelf_MonoTwice)(benchmark::State& state) {
    sfz::Filter filterLeft;
    sfz::Filter filterRight;
    filterLeft.setType(sfz::FilterType::kFilterLpf2p);
    filterRight.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto pkshIterator = pksh.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            filterLeft.process( &inLIterator.base(), &outLIterator.base(), *cutoffIterator, *qIterator, *pkshIterator, step);
            filterRight.process(&inRIterator.base(), &outRIterator.base(), *cutoffIterator, *qIterator, *pkshIterator, step);
            cutoffIterator += step;
            qIterator += step;
            pkshIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_DEFINE_F(FilterFixture, Shelf_Stereo)(benchmark::State& state) {
    sfz::Filter filter;
    filter.setChannels(2);
    filter.setType(sfz::FilterType::kFilterLpf2p);
    for (auto _ : state)
    {
        const auto step = static_cast<size_t>(state.range(0));
        auto cutoffIterator = cutoff.begin();
        auto qIterator = q.begin();
        auto pkshIterator = pksh.begin();
        auto inLIterator = inputLeft.begin();
        auto inRIterator = inputRight.begin();
        auto outRIterator = outputLeft.begin();
        auto outLIterator = outputRight.begin();
        while (cutoffIterator < cutoff.end())
        {
            float * inputs[2] = { inLIterator.base(), inRIterator.base() };
            float * outputs[2] = { outLIterator.base(), outRIterator.base() };
            filter.process(inputs, outputs, *cutoffIterator, *qIterator, *pkshIterator, step);
            cutoffIterator += step;
            qIterator += step;
            pkshIterator += step;
            inLIterator += step;
            inRIterator += step;
            outLIterator += step;
            outRIterator += step;
        }
    }
}

BENCHMARK_REGISTER_F(FilterFixture, OnePole_MonoOnce)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, OnePole_MonoTwice)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, OnePole_Stereo)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, TwoPole_MonoOnce)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, TwoPole_MonoTwice)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, TwoPole_Stereo)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, Shelf_MonoOnce)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, Shelf_MonoTwice)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_REGISTER_F(FilterFixture, Shelf_Stereo)->RangeMultiplier(2)->Range(1, 1 << 8);
BENCHMARK_MAIN();

