// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Panning.h"
#include "simd/Common.h"
#include <benchmark/benchmark.h>
#include <random>
#include <absl/algorithm/container.h>
#include "absl/types/span.h"
#include <arm_neon.h>

#include <jsl/allocator>
template <class T, std::size_t A = 16>
using aligned_vector = std::vector<T, jsl::aligned_allocator<T, A>>;

// Number of elements in the table, odd for equal volume at center
constexpr int panSize = 4095;

// Table of pan values for the left channel, extra element for safety
static const auto panData = []()
{
    std::array<float, panSize + 1> pan;
    int i = 0;

    for (; i < panSize; ++i)
        pan[i] = std::cos(i * (piTwo<double>() / (panSize - 1)));

    for (; i < static_cast<int>(pan.size()); ++i)
        pan[i] = pan[panSize - 1];

    return pan;
}();

float _panLookup(float pan)
{
    // reduce range, round to nearest
    int index = lroundPositive(pan * (panSize - 1));
    return panData[index];
}

void panScalar(const float* panEnvelope, float* leftBuffer, float* rightBuffer, unsigned size) noexcept
{
    const auto sentinel = panEnvelope + size;
    while (panEnvelope < sentinel) {
        auto p =(*panEnvelope + 1.0f) * 0.5f;
        p = clamp(p, 0.0f, 1.0f);
        *leftBuffer *= _panLookup(p);
        *rightBuffer *= _panLookup(1 - p);
        incrementAll(panEnvelope, leftBuffer, rightBuffer);
    }
}

void panSIMD(const float* panEnvelope, float* leftBuffer, float* rightBuffer, unsigned size) noexcept
{
    const auto sentinel = panEnvelope + size;
    int32_t indices[4];
    while (panEnvelope < sentinel) {
        float32x4_t mmPan = vld1q_f32(panEnvelope);
        mmPan = vaddq_f32(mmPan, vdupq_n_f32(1.0f));
        mmPan = vmulq_n_f32(mmPan, 0.5f * panSize);
        mmPan = vaddq_f32(mmPan, vdupq_n_f32(0.5f));
        mmPan = vminq_f32(mmPan, vdupq_n_f32(panSize));
        mmPan = vmaxq_f32(mmPan, vdupq_n_f32(0.0f));
        int32x4_t mmIdx = vcvtq_s32_f32(mmPan);
        vst1q_s32(indices, mmIdx);

        leftBuffer[0] *= panData[indices[0]];
        rightBuffer[0] *= panData[panSize - indices[0] - 1];
        leftBuffer[1] *= panData[indices[1]];
        rightBuffer[1] *= panData[panSize - indices[1]- 1];
        leftBuffer[2] *= panData[indices[2]];
        rightBuffer[2] *= panData[panSize - indices[2]- 1];
        leftBuffer[3] *= panData[indices[3]];
        rightBuffer[3] *= panData[panSize - indices[3]- 1];

        incrementAll<4>(panEnvelope, leftBuffer, rightBuffer);
    }
}

class PanFixture : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
    pan.resize(state.range(0));
    right.resize(state.range(0));
    left.resize(state.range(0));

    if (!willAlign<16>(pan.data(), left.data(), right.data()))
        std::cout << "Will not align!" << '\n';
    absl::c_generate(pan, [&]() { return dist(gen); });
    absl::c_generate(left, [&]() { return dist(gen); });
    absl::c_generate(right, [&]() { return dist(gen); });
  }

  void TearDown(const ::benchmark::State& /* state */) {

  }

    aligned_vector<float> pan;
    aligned_vector<float> right;
    aligned_vector<float> left;
};

BENCHMARK_DEFINE_F(PanFixture, PanScalar)(benchmark::State& state) {
    for (auto _ : state)
    {
        panScalar(pan.data(), left.data(), right.data(), state.range(0));
    }
}

BENCHMARK_DEFINE_F(PanFixture, PanSIMD)(benchmark::State& state) {
    for (auto _ : state)
    {
        panSIMD(pan.data(), left.data(), right.data(), state.range(0));
    }
}

BENCHMARK_DEFINE_F(PanFixture, PanSfizz)(benchmark::State& state) {
    for (auto _ : state)
    {
        sfz::pan(pan.data(), left.data(), right.data(), state.range(0));
    }
}

// Register the function as a benchmark
BENCHMARK_REGISTER_F(PanFixture, PanScalar)->RangeMultiplier(4)->Range((1 << 4), (1 << 12));
BENCHMARK_REGISTER_F(PanFixture, PanSIMD)->RangeMultiplier(4)->Range((1 << 4), (1 << 12));
BENCHMARK_REGISTER_F(PanFixture, PanSfizz)->RangeMultiplier(4)->Range((1 << 4), (1 << 12));
BENCHMARK_MAIN();
