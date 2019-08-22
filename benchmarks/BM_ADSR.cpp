#include <benchmark/benchmark.h>
#include "../sources/ADSREnvelope.h"
#include <algorithm>
#include <random>
#include <numeric>

constexpr int fixedAmount { 12 };
constexpr int envelopeSize { 2 << 16 };
constexpr int attack = static_cast<int>(envelopeSize / 4) - fixedAmount;
constexpr int decay = attack;
constexpr int release = attack;
constexpr int releaseTime = envelopeSize - static_cast<int>(envelopeSize / 4);


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
    state.counters["Per block"] = benchmark::Counter(envelopeSize / state.range(0), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
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

    state.counters["Per block"] = benchmark::Counter(envelopeSize / state.range(0), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
}

BENCHMARK(Point)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK(Block)->RangeMultiplier(2)->Range((2<<6), (2<<11));
BENCHMARK_MAIN();