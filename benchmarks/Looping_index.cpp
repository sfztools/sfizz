#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "../sources/Intrinsics.h"
#include "../sources/Buffer.h"
constexpr int loopOffset { 5 };
constexpr int loopPoint { 51 };
constexpr int loopBack { loopPoint - loopOffset };
constexpr float maxJump { 4 };

static void Straight(benchmark::State& state) {
    Buffer<int> indices(state.range(0));
    Buffer<float> leftCoeffs(state.range(0));
    Buffer<float> rightCoeffs(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, maxJump };
    for (auto _ : state)
    {
        auto offset = dist(gen);
        float floatingIndex = 0.0f;
        auto index = indices.data();
        auto leftCoeff = leftCoeffs.data();
        auto rightCoeff = rightCoeffs.data();
        while (index < indices.end())
        {
            *index = static_cast<int>(floatingIndex);
            *rightCoeff = floatingIndex - *index;
            *leftCoeff = 1.0f - *rightCoeff;
            floatingIndex += offset;
            if (floatingIndex > loopPoint)
                floatingIndex -= loopBack;
            index++;
            leftCoeff++;
            rightCoeff++;
        }
    }
}

static void SIMD(benchmark::State& state) {
    Buffer<int> indices(state.range(0));
    Buffer<float> leftCoeffs(state.range(0));
    Buffer<float> rightCoeffs(state.range(0));
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0, maxJump };
    for (auto _ : state)
    {
        auto index = indices.begin();
        auto leftCoeff = leftCoeffs.begin();
        auto rightCoeff = rightCoeffs.begin();
        const auto alignedEnd = state.range(0) - (state.range(0) & 3);
        float floatingIndex = 48.0f;
        auto offset = dist(gen);
        const auto wrappingRegister   = _mm_set1_ps(loopBack);
        const auto upperBoundRegister = _mm_set1_ps(loopPoint);
        const auto offsetRegister = _mm_set1_ps(offset);
        const auto incrementRegister = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
        while (index < indices.data() + alignedEnd)
        {
            auto floatingIndexRegister = _mm_set_ps1(floatingIndex);
            floatingIndexRegister = _mm_add_ps(floatingIndexRegister, _mm_mul_ps(incrementRegister, offsetRegister));
            auto rightCoefficientsRegister = _mm_sub_ps(floatingIndexRegister, 
                _mm_cvtepi32_ps(_mm_cvtps_epi32(_mm_sub_ps(floatingIndexRegister, _mm_set_ps1(0.5f))))
            );
            auto leftCoefficientsRegister = _mm_sub_ps(_mm_set_ps1(1.0f), rightCoefficientsRegister);
            const auto comparisonRegister = _mm_cmpge_ps(floatingIndexRegister, upperBoundRegister);
            auto loopbackRegister = _mm_sub_ps(floatingIndexRegister, wrappingRegister);
            loopbackRegister = _mm_and_ps(comparisonRegister, loopbackRegister);
            floatingIndexRegister = _mm_andnot_ps(comparisonRegister, floatingIndexRegister);
            floatingIndexRegister = _mm_add_ps(floatingIndexRegister, loopbackRegister);
            auto indexRegister = _mm_cvtps_epi32(floatingIndexRegister);
            _mm_store_si128(reinterpret_cast<__m128i*>(index), indexRegister);
            _mm_store_ps(leftCoeff, leftCoefficientsRegister);
            _mm_store_ps(rightCoeff, rightCoefficientsRegister);
            floatingIndex = _mm_cvtss_f32(_mm_shuffle_ps(floatingIndexRegister, floatingIndexRegister, _MM_SHUFFLE(0, 0, 0, 3)));;
            // floatingIndex = *(index + 3) + *(rightCoeff + 3);
            index += 4;
            leftCoeff += 4;
            rightCoeff += 4;
        }
        while (index < indices.end())
        {
            *index = static_cast<int>(floatingIndex);
            *rightCoeff = floatingIndex - *index;
            *leftCoeff = 1.0f - *rightCoeff;
            floatingIndex += offset;
            if (floatingIndex > loopPoint)
                floatingIndex -= loopBack;
            index++;
            leftCoeff++;
            rightCoeff++;
        }
    }
}

// Register the function as a benchmark
BENCHMARK(Straight)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK(SIMD)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();