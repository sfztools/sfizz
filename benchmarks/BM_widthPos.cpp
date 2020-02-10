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
#include "Config.h"
#include "ScopedFTZ.h"
#include "absl/types/span.h"

class WidthPosArray : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<float> dist { 0.001f, 1.0f };
    width = std::vector<float>(state.range(0));
    position = std::vector<float>(state.range(0));
    left = std::vector<float>(state.range(0));
    right = std::vector<float>(state.range(0));
    std::generate(width.begin(), width.end(), [&]() { return dist(gen); });
    std::generate(position.begin(), position.end(), [&]() { return dist(gen); });
    std::generate(right.begin(), right.end(), [&]() { return dist(gen); });
    std::generate(left.begin(), left.end(), [&]() { return dist(gen); });
    temp1 = std::vector<float>(state.range(0));
    temp2 = std::vector<float>(state.range(0));
    temp3 = std::vector<float>(state.range(0));
    span1 = absl::MakeSpan(temp1);
    span2 = absl::MakeSpan(temp2);
    span3 = absl::MakeSpan(temp3);
  }

  void TearDown(const ::benchmark::State& state [[maybe_unused]]) {

  }

  std::vector<float> width;
  std::vector<float> position;
  std::vector<float> left;
  std::vector<float> right;
  std::vector<float> temp1;
  std::vector<float> temp2;
  std::vector<float> temp3;
  absl::Span<float> span1;
  absl::Span<float> span2;
  absl::Span<float> span3;
};

BENCHMARK_DEFINE_F(WidthPosArray, Scalar)(benchmark::State& state) {
    ScopedFTZ ftz;
    for (auto _ : state)
    {
        auto widthPtr = width.data();
        auto posPtr = position.data();
        auto leftPtr = left.data();
        auto rightPtr = right.data();
        const auto sentinel = width.data() + width.size();
        while(widthPtr < sentinel)
        {
            auto mid = (*leftPtr + *rightPtr) * sqrtTwoInv<float>;
            auto side = (*leftPtr - *rightPtr) * sqrtTwoInv<float>;
            sfz::_internals::snippetWidth(*widthPtr, mid, side);
            auto midRight = mid;
            sfz::_internals::snippetPan(*posPtr, mid, midRight);
            *leftPtr = (mid + side) * sqrtTwoInv<float>;
            *rightPtr = (midRight - side) * sqrtTwoInv<float>;
            incrementAll(leftPtr, rightPtr, widthPtr, posPtr);
        }
    }
}

BENCHMARK_DEFINE_F(WidthPosArray, BlockOps)(benchmark::State& state) {
    ScopedFTZ ftz;
    const auto leftBuffer = absl::MakeSpan(left);
    const auto rightBuffer = absl::MakeSpan(right);
    for (auto _ : state)
    {
        const auto leftBufferCopy = span2;
        sfz::copy<float>(left, leftBufferCopy);

        const auto midBuffer = leftBuffer;
        sfz::add<float>(rightBuffer, midBuffer);

        const auto sideBuffer = rightBuffer;
        sfz::applyGain<float>(-1.0f, sideBuffer);
        sfz::add<float>(leftBufferCopy, sideBuffer);

        sfz::applyGain<float>(sqrtTwoInv<float>, midBuffer);
        sfz::applyGain<float>(sqrtTwoInv<float>, sideBuffer);

        // Apply the width process
        sfz::width<float>(width, midBuffer, sideBuffer);

        // Copy the mid channel into another span
        const auto midBufferCopy = span3;
        sfz::copy<float>(midBuffer, midBufferCopy);
        sfz::pan<float>(position, midBuffer, midBufferCopy);

        // Rebuild left/right
        // Recall that midBuffer and leftBuffer point to the same buffer
        sfz::add<float>(sideBuffer, leftBuffer);
        sfz::applyGain(sqrtTwoInv<float>, leftBuffer);

        // Recall that sideBuffer and rightBuffer point to the same buffer
        sfz::applyGain<float>(-1.0f, sideBuffer);
        sfz::add<float>(midBufferCopy, sideBuffer);
        sfz::applyGain(sqrtTwoInv<float>, rightBuffer);
    }
}

BENCHMARK_REGISTER_F(WidthPosArray, Scalar)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_REGISTER_F(WidthPosArray, BlockOps)->RangeMultiplier(4)->Range(1 << 2, 1 << 12);
BENCHMARK_MAIN();
