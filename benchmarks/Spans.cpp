#include <benchmark/benchmark.h>
#include "gsl/gsl-lite.hpp"
#include "absl/types/span.h"
#include <random>
#include <algorithm>

constexpr float filterGain { 0.25f };

void processRaw(float* input, float* lowpass, float gain, int numSamples)
{
    const auto end = input + numSamples;
    float state = 0.0f;
    float intermediate;
    const auto G = gain / (1 - gain);
    while (input < end)
    {
        intermediate = G * (*input++ - state);
        *lowpass = intermediate + state;
        state = *lowpass++ + intermediate;
    }
}

void processGSLSpan(gsl::span<const float> input, gsl::span<float> lowpass, float gain)
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

void processABSLSpan(absl::Span<const float> input, absl::Span<float> lowpass, float gain)
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

static void Raw(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
    {
        processRaw(input.data(), output.data(), filterGain, state.range(0));
    }
}

static void GSLSpan(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
    {
        processGSLSpan(input, output, filterGain);
    }
}

static void ABSLSpan(benchmark::State& state) {
    std::vector<float> input(state.range(0));
    std::vector<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    
    std::normal_distribution<float> dist { };
    std::generate(input.begin(), input.end(), [&]() {
        return dist(gen);
    });

    for (auto _ : state)
    {
        processABSLSpan(input, absl::MakeSpan(output), filterGain);
    }
}

// Register the function as a benchmark

BENCHMARK(Raw)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK(GSLSpan)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK(ABSLSpan)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();