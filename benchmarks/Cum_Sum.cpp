#include <benchmark/benchmark.h>
#include <random>
#include "../sources/Intrinsics.h"
#include "../sources/Buffer.h"


static void Dummy(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        benchmark::DoNotOptimize(value);
    }
}

static void Straight(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        for (auto& out: output)
        {
            out = value;
            value *= value;
        }
    }
}

static void SIMD(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        auto out = output.begin();
        const auto alignedEnd = state.range(0) - (state.range(0) & 3);

        auto baseReg = _mm_set_ps1(value);
        auto prodReg = _mm_set_ps(value+value+value+value, value+value+value, value+value, value);

        while (out < output.data() + alignedEnd)
        {
            baseReg = _mm_add_ps(baseReg, prodReg);
            _mm_storeu_ps(out, baseReg);
            baseReg = _mm_shuffle_ps(baseReg, baseReg, _MM_SHUFFLE(3, 3, 3, 3));
            out += 4;
        }

        while (out < output.end())
        {
            *out++ = value;
            value *= value;
        }
    }
}

static void SIMD_unaligned(benchmark::State& state) {
    Buffer<float> output(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, 1 };
    for (auto _ : state)
    {
        auto value = dist(gen);
        auto out = output.begin() + 1;
        const auto alignedEnd = state.range(0) - (state.range(0) & 3);

        auto baseReg = _mm_set_ps1(value);
        auto prodReg = _mm_set_ps(value+value+value+value, value+value+value, value+value, value);

        while (out < output.data() + alignedEnd)
        {
            baseReg = _mm_add_ps(baseReg, prodReg);
            _mm_storeu_ps(out, baseReg);
            baseReg = _mm_shuffle_ps(baseReg, baseReg, _MM_SHUFFLE(3, 3, 3, 3));
            out += 4;
        }

        while (out < output.end())
        {
            *out++ = value;
            value *= value;
        }
    }
}

// Register the function as a benchmark
BENCHMARK(Dummy)->RangeMultiplier(2)->Range((1 << 2), (1 << 8));
BENCHMARK(Straight)->RangeMultiplier(2)->Range((1 << 2), (1 << 8));
BENCHMARK(SIMD)->RangeMultiplier(2)->Range((1 << 2), (1 << 8));
BENCHMARK(SIMD_unaligned)->RangeMultiplier(2)->Range((1 << 2), (1 << 8));
BENCHMARK_MAIN();