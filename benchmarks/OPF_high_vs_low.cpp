#include <benchmark/benchmark.h>
#include "gsl/gsl-lite.hpp"
#include <random>
#include <algorithm>
#include "../sources/Intrinsics.h"

constexpr float filterGain { 0.25f };

void lowpass(gsl::span<const float> input, gsl::span<float> lowpass, float gain)
{
    float state = 0.0f;
    float intermediate;
    const auto G = gain / (1 - gain);
    for (auto [in, out] = std::make_pair(input.begin(), lowpass.begin()); 
         in < input.end() && out < lowpass.end();
         in++, out++)
    {
        intermediate = G * (*in - state);
        *out = intermediate + state;
        state = *out + intermediate;
    }
}

void highpass(gsl::span<const float> input, gsl::span<float> highpass, float gain)
{
    float state = 0.0f;
    float intermediate;
    const auto G = gain / (1 - gain);
    for (auto [in, out] = std::make_pair(input.begin(), highpass.begin()); 
         in < input.end() && out < highpass.end();
         in++, out++)
    {
        intermediate = G * (*in - state);
        *out = *in - intermediate - state;
        state += 2*intermediate;
    }
}

void highpass_foreach(gsl::span<const float> input, gsl::span<float> highpass, float gain)
{
    lowpass(input, highpass, gain);
    for (auto [in, out] = std::make_pair(input.begin(), highpass.begin()); 
         in < input.end() && out < highpass.end();
         in++, out++)
        *out = *in - *out;
}

void highpass_raw(gsl::span<const float> input, gsl::span<float> highpass, float gain)
{
    lowpass(input, highpass, gain);
    auto in = input.data();
    auto out = highpass.data();
    while (in < input.end() && out < highpass.end())
    {
        *out = *in - *out;
        out++;
        in++;
    }
}

void highpass_sse(gsl::span<const float> input, gsl::span<float> highpass, float gain)
{
    lowpass(input, highpass, gain);
    auto in = input.data();
    auto out = highpass.data();
    constexpr int FloatAlignment { 4 };
    const auto inputAlignedEnd = input.data() + (input.size() - (input.size() & (FloatAlignment - 1)));
    const auto outputAlignedEnd = highpass.data() + (highpass.size() - (highpass.size() & (FloatAlignment - 1)));
    while (in < inputAlignedEnd && out < outputAlignedEnd)
    {
        const auto inputRegister = _mm_loadu_ps(in);
        auto outputRegister = _mm_loadu_ps(out);
        outputRegister = _mm_sub_ps(inputRegister, outputRegister);
        _mm_storeu_ps(out, outputRegister);
        in += 4;
        out += 4;
    }
    while (in < input.end() && out < highpass.end())
    {
        *out = *in - *out;
        out++;
        in++;
    }
}


static void Low(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        lowpass(input, output, filterGain);
}

static void High(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass(input, output, filterGain);
}

static void High_ForEach(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass_foreach(input, output, filterGain);
}

static void High_Raw(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass_raw(input, output, filterGain);
}

static void High_SSE(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
        highpass_sse(input, output, filterGain);
}


BENCHMARK(Low)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High_ForEach)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High_Raw)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK(High_SSE)->RangeMultiplier(2)->Range((2<<5), (2<<10));
BENCHMARK_MAIN();