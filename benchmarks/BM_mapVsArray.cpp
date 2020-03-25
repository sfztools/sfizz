// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
#include <random>
#include "../src/sfizz/CCMap.h"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>

constexpr int maxCC { 256 };

class MyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };

        std::array<int, maxCC> sourceCC;
        absl::c_iota(sourceCC, 0);
        absl::c_shuffle(sourceCC, gen);
        ccs.resize(state.range(0));
        std::copy(sourceCC.begin(), sourceCC.begin() + state.range(0), ccs.begin());
        std::uniform_real_distribution<float> distFloat { 0.1f, 1.0f };
        vector.resize(maxCC);
        absl::c_generate(vector, [&]() {
            return distFloat(gen);
        });
        for (unsigned i = 0; i < state.range(0); ++i) {
            map[ccs[i]] = vector[ccs[i]];
        }
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    std::vector<int> ccs;
    std::vector<float> vector;
    sfz::CCMap<float> map { 1 };
};

BENCHMARK_DEFINE_F(MyFixture, ArraySearch)
(benchmark::State& state)
{
    float value { 1 };
    for (auto _ : state) {
        for (unsigned i = 0; i < state.range(0); ++i) {
            value *= vector[ccs[i]];
        }
        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK_DEFINE_F(MyFixture, MapSearch)
(benchmark::State& state)
{
    float value { 0 };
    for (auto _ : state) {
        for (unsigned i = 0; i < state.range(0); ++i) {
            value *= map[ccs[i]];
        }
        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK_REGISTER_F(MyFixture, ArraySearch)->Range(4, maxCC);
BENCHMARK_REGISTER_F(MyFixture, MapSearch)->Range(4, maxCC);
BENCHMARK_MAIN();
