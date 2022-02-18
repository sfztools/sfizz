// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
#include <random>
#include "../src/sfizz/Range.h"
#include <absl/container/flat_hash_map.h>
#include <absl/algorithm/container.h>

constexpr int maxCC { 256 };

class MyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state)
    {
        std::random_device rd {};
        std::mt19937 gen { rd() };
        std::uniform_real_distribution<float> distFloat { 0.1f, 1.0f };
        std::uniform_int_distribution<int> distInt { 1, maxCC };
        floats = std::vector<float>(state.range(0));
        ccs = std::vector<int>(state.range(0));
        ranges = std::vector<sfz::Range<int>>(state.range(0));
        absl::c_generate(floats, [&]() {
            return distFloat(gen);
        });
        absl::c_generate(ccs, [&]() {
            return distInt(gen);
        });
        absl::c_generate(ranges, [&]() {
            return sfz::Range<int>(distInt(gen), distInt(gen));
        });
    }

    void TearDown(const ::benchmark::State& /* state */)
    {
    }

    std::vector<int> ccs;
    std::vector<sfz::Range<int>> ranges;
    std::vector<float> floats;
};

template<class ValueType>
struct CCValuePair {
    int cc;
    ValueType value;
};

template<class ValueType, bool CompareValue = false>
struct CCValuePairComparator {
    bool operator()(const CCValuePair<ValueType>& valuePair, const int& cc)
    {
        return (valuePair.cc < cc);
    }

    bool operator()(const int& cc, const CCValuePair<ValueType>& valuePair)
    {
        return (cc < valuePair.cc);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.cc < rhs.cc);
    }
};

template<class ValueType>
struct CCValuePairComparator<ValueType, true> {
    bool operator()(const CCValuePair<ValueType>& valuePair, const ValueType& value)
    {
        return (valuePair.value < value);
    }

    bool operator()(const ValueType& value, const CCValuePair<ValueType>& valuePair)
    {
        return (value < valuePair.value);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.value < rhs.value);
    }
};

template <class ValueType>
class CCMap {
public:
    CCMap() = delete;
    /**
     * @brief Construct a new CCMap object with the specified default value.
     *
     * @param defaultValue
     */
    CCMap(const ValueType& defaultValue)
        : defaultValue(defaultValue)
    {
    }
    CCMap(CCMap&&) = default;
    CCMap(const CCMap&) = default;
    ~CCMap() = default;

    /**
     * @brief Returns the held object at the index, or a default value if not present
     *
     * @param index
     * @return const ValueType&
     */
    const ValueType& getWithDefault(int index) const noexcept
    {
        auto it = absl::c_lower_bound(container, index, CCValuePairComparator<ValueType>{});
        if (it == container.end() || it->cc != index) {
            return defaultValue;
        } else {
            return it->value;
        }
    }

    /**
     * @brief Get the value at index or emplace a new one if not present
     *
     * @param index the index of the element
     * @return ValueType&
     */
    ValueType& operator[](const int& index) noexcept
    {
        auto it = absl::c_lower_bound(container, index, CCValuePairComparator<ValueType>{});
        if (it == container.end() || it->cc != index) {
            auto inserted = container.insert(it, { index, defaultValue });
            return inserted->value;
        } else {
            return it->value;
        }
    }

    /**
     * @brief Is the container empty
     *
     * @return true
     * @return false
     */
    inline bool empty() const { return container.empty(); }
    /**
     * @brief Returns true if the container containers an element at index
     *
     * @param index
     * @return true
     * @return false
     */
    bool contains(int index) const noexcept
    {
        return absl::c_binary_search(container, index, CCValuePairComparator<ValueType>{});
    }
    typename std::vector<CCValuePair<ValueType>>::const_iterator begin() const { return container.cbegin(); }
    typename std::vector<CCValuePair<ValueType>>::const_iterator end() const { return container.cend(); }
private:
    // typename std::vector<std::pair<int, ValueType>>::iterator begin() { return container.begin(); }
    // typename std::vector<std::pair<int, ValueType>>::iterator end() { return container.end(); }

    const ValueType defaultValue;
    std::vector<CCValuePair<ValueType>> container;
};

BENCHMARK_DEFINE_F(MyFixture, FillVector_Float)
(benchmark::State& state)
{
    for (auto _ : state) {
        CCMap<float> map { 0 };
        for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = floats[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, FillVector_Range)
(benchmark::State& state)
{
    for (auto _ : state) {
        CCMap<sfz::Range<int>> map { sfz::Range<int>(0, 127) };
        for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = ranges[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, FillAbseilFlatHM_Float)
(benchmark::State& state)
{
    for (auto _ : state) {
        absl::flat_hash_map<int, float> map;
        for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = floats[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, FillAbseilFlatHM_Range)
(benchmark::State& state)
{
    for (auto _ : state) {
        absl::flat_hash_map<int, sfz::Range<int>> map;
        for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = ranges[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupBaseline_Float)
(benchmark::State& state)
{
    std::vector<float> output;
    output.resize(state.range(0));

    std::vector<float> map;
    output.reserve(state.range(0));
    for (int i = 0; i < state.range(0); ++i)
        map.push_back(floats[i]);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupBaseline_Range)
(benchmark::State& state)
{
    std::vector<sfz::Range<int>> output;
    output.resize(state.range(0));

    std::vector<sfz::Range<int>> map;
    output.reserve(state.range(0));
    for (int i = 0; i < state.range(0); ++i)
        map.push_back(ranges[i]);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[i];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupVector_Float)
(benchmark::State& state)
{
    std::vector<float> output;
    output.resize(state.range(0));

    CCMap<float> map { 0 };
    for (int i = 0; i < state.range(0); ++i)
        map[ccs[i]] = floats[i];

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[ccs[i]];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupVector_Range)
(benchmark::State& state)
{
    std::vector<sfz::Range<int>> output;
    output.resize(state.range(0));

    CCMap<sfz::Range<int>> map { sfz::Range<int>(0, 127) };
    for (int i = 0; i < state.range(0); ++i)
        map[ccs[i]] = ranges[i];

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[ccs[i]];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupAbseilFlatHM_Float)
(benchmark::State& state)
{
    std::vector<float> output;
    output.resize(state.range(0));

    absl::flat_hash_map<int, float> map;
    for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = floats[i];
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[ccs[i]];
    }
}

BENCHMARK_DEFINE_F(MyFixture, LookupAbseilFlatHM_Range)
(benchmark::State& state)
{
    std::vector<sfz::Range<int>> output;
    output.resize(state.range(0));

    absl::flat_hash_map<int, sfz::Range<int>> map;
    for (int i = 0; i < state.range(0); ++i)
        map[ccs[i]] = ranges[i];

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i)
            output[i] = map[ccs[i]];
    }
}


BENCHMARK_DEFINE_F(MyFixture, IterateVector_Float)
(benchmark::State& state)
{
    std::vector<float> output;
    output.reserve(maxCC);

    CCMap<float> map { 0 };
    for (int i = 0; i < state.range(0); ++i)
        map[ccs[i]] = floats[i];

    for (auto _ : state) {
        for (auto& pair: map)
            output.push_back(pair.value);
    }
}

BENCHMARK_DEFINE_F(MyFixture, IterateAbseilFlatHM_Float)
(benchmark::State& state)
{
    std::vector<float> output;
    output.reserve(maxCC);

    absl::flat_hash_map<int, float> map;
    for (int i = 0; i < state.range(0); ++i)
            map[ccs[i]] = floats[i];
    for (auto _ : state) {
        for (auto& pair: map)
            output.push_back(pair.second);
    }
}


BENCHMARK_REGISTER_F(MyFixture, FillVector_Float)->RangeMultiplier(2)->Range(16, 512);
// BENCHMARK_REGISTER_F(MyFixture, FillVector_Range)->RangeMultiplier(2)->Range(16, 512);
BENCHMARK_REGISTER_F(MyFixture, FillAbseilFlatHM_Float)->RangeMultiplier(2)->Range(16, 512);
// BENCHMARK_REGISTER_F(MyFixture, FillAbseilFlatHM_Range)->RangeMultiplier(2)->Range(16, 512);
BENCHMARK_REGISTER_F(MyFixture, LookupBaseline_Float)->RangeMultiplier(2)->Range(16, 512);
// BENCHMARK_REGISTER_F(MyFixture, LookupBaseline_Range)->RangeMultiplier(2)->Range(16, 512);
BENCHMARK_REGISTER_F(MyFixture, LookupVector_Float)->RangeMultiplier(2)->Range(16, 512);
// BENCHMARK_REGISTER_F(MyFixture, LookupVector_Range)->RangeMultiplier(2)->Range(16, 512);
BENCHMARK_REGISTER_F(MyFixture, LookupAbseilFlatHM_Float)->RangeMultiplier(2)->Range(16, 512);
// BENCHMARK_REGISTER_F(MyFixture, LookupAbseilFlatHM_Range)->RangeMultiplier(2)->Range(16, 512);
BENCHMARK_REGISTER_F(MyFixture, IterateVector_Float)->Range(maxCC, maxCC);
BENCHMARK_REGISTER_F(MyFixture, IterateAbseilFlatHM_Float)->Range(maxCC, maxCC);
BENCHMARK_MAIN();
