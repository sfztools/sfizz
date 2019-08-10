#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <numeric>
#include "../sources/Intrinsics.h"
#include "../sources/Buffer.h"

// In this one we have an array of indices

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
    Buffer<float> jumps(state.range(0));
    std::generate(jumps.begin(), jumps.end(), [&]() { return dist(gen); });

    for (auto _ : state)
    {
        float floatingIndex = 0.0f;
        auto index = indices.data();
        auto leftCoeff = leftCoeffs.data();
        auto rightCoeff = rightCoeffs.data();
        auto jump = jumps.data();
        
        while (index < indices.end())
        {
            *index = static_cast<int>(floatingIndex);
            *rightCoeff = floatingIndex - *index;
            *leftCoeff = 1.0f - *rightCoeff;
            floatingIndex += *jump;
            if (floatingIndex > loopPoint)
                floatingIndex -= loopBack;
            index++;
            leftCoeff++;
            rightCoeff++;
            jump++;
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
    Buffer<float> jumps(state.range(0));
    std::generate(jumps.begin(), jumps.end(), [&]() { return dist(gen); });

    for (auto _ : state)
    {
        auto index = indices.data();
        auto leftCoeff = leftCoeffs.data();
        auto rightCoeff = rightCoeffs.data();
        auto jump = jumps.data();

        const auto alignedEnd = state.range(0) - (state.range(0) & 3);
        
        auto floatingIndexReg = _mm_set_ps1(48.0f);
        const auto wrappingReg   = _mm_set1_ps(loopBack);
        const auto upperBoundReg = _mm_set1_ps(loopPoint);

        while (index < indices.data() + alignedEnd)
        {
            auto offsetReg = _mm_load_ps(jump);
            offsetReg = _mm_add_ps(offsetReg, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(offsetReg), 4))); 
            offsetReg = _mm_add_ps(offsetReg, _mm_shuffle_ps(_mm_setzero_ps(), offsetReg, 0x40));

            floatingIndexReg = _mm_add_ps(floatingIndexReg, offsetReg);
            const auto comparisonReg = _mm_cmpge_ps(floatingIndexReg, upperBoundReg);
            auto loopbackReg = _mm_sub_ps(floatingIndexReg, wrappingReg);
            loopbackReg = _mm_and_ps(comparisonReg, loopbackReg);
            floatingIndexReg = _mm_andnot_ps(comparisonReg, floatingIndexReg);
            floatingIndexReg = _mm_add_ps(floatingIndexReg, loopbackReg);

            auto indexReg = _mm_cvtps_epi32(_mm_sub_ps(floatingIndexReg, _mm_set_ps1(0.5f)));
            _mm_store_si128(reinterpret_cast<__m128i*>(index), indexReg);
            
            auto rightCoefficientsReg = _mm_sub_ps(floatingIndexReg, _mm_cvtepi32_ps(indexReg));
            auto leftCoefficientsReg = _mm_sub_ps(_mm_set_ps1(1.0f), rightCoefficientsReg);
            _mm_store_ps(leftCoeff, leftCoefficientsReg);
            _mm_store_ps(rightCoeff, rightCoefficientsReg);

            floatingIndexReg = _mm_shuffle_ps(floatingIndexReg, floatingIndexReg, _MM_SHUFFLE(3, 3, 3, 3));
            // floatingIndex = _mm_cvtss_f32(_mm_shuffle_ps(floatingIndexReg, floatingIndexReg, _MM_SHUFFLE(0, 0, 0, 3)));;
            // floatingIndex = *(index + 3) + *(rightCoeff + 3);
            index += 4;
            jump += 4;
            leftCoeff += 4;
            rightCoeff += 4;
        }

        float floatingIndex = _mm_cvtss_f32(floatingIndexReg);
        while (index < indices.end())
        {
            *index = static_cast<int>(floatingIndex);
            *rightCoeff = floatingIndex - *index;
            *leftCoeff = 1.0f - *rightCoeff;
            floatingIndex += *jump;
            if (floatingIndex > loopPoint)
                floatingIndex -= loopBack;
            index++;
            leftCoeff++;
            rightCoeff++;
            jump++;
        }
    }
}

// Register the function as a benchmark
BENCHMARK(Straight)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK(SIMD)->RangeMultiplier(2)->Range((2<<6), (2<<12));
BENCHMARK_MAIN();