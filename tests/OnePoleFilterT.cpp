// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "sfizz/OnePoleFilter.h"
#include "catch2/catch.hpp"
#include "cnpy.h"
#include "ghc/fs_std.hpp"
#include <absl/types/span.h>
#include <algorithm>
#include <string>
using namespace Catch::literals;

template <class Type>
inline bool approxEqual(const std::vector<Type>& lhs, const std::vector<Type>& rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (lhs[i] != Approx(rhs[i]).epsilon(1e-3)) {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

template <class Type>
void testLowpass(const fs::path& inputNumpyFile, const fs::path& outputNumpyFile, Type gain)
{
    const auto input = cnpy::npy_load(inputNumpyFile.string());
    REQUIRE(input.word_size == 8);
    const auto inputSpan = absl::MakeSpan(input.data<double>(), input.shape[0]);

    const auto output = cnpy::npy_load(outputNumpyFile.string());
    REQUIRE(output.word_size == 8);
    const auto outputSpan = absl::MakeSpan(output.data<double>(), output.shape[0]);
    auto size = std::min(outputSpan.size(), inputSpan.size());
    REQUIRE(size > 0);

    std::vector<Type> inputData;
    std::vector<Type> expectedData;
    inputData.reserve(size);
    expectedData.reserve(size);
    for (auto& data : inputSpan)
        inputData.push_back(static_cast<Type>(data));
    for (auto& data : outputSpan)
        expectedData.push_back(static_cast<Type>(data));

    sfz::OnePoleFilter<Type> filter { gain };
    std::vector<Type> outputData(size);
    filter.processLowpass(inputData, absl::MakeSpan(outputData));
    REQUIRE(approxEqual(outputData, expectedData));

    filter.reset();
    std::fill(outputData.begin(), outputData.end(), 0.0f);
    std::vector<Type> gains(size);
    std::fill(gains.begin(), gains.end(), gain);
    filter.processLowpassVariableGain(inputData, absl::MakeSpan(outputData), gains);
    REQUIRE(approxEqual(outputData, expectedData));
}

template <class Type>
void testHighpass(const fs::path& inputNumpyFile, const fs::path& outputNumpyFile, Type gain)
{
    const auto input = cnpy::npy_load(inputNumpyFile.string());
    REQUIRE(input.word_size == 8);
    const auto inputSpan = absl::MakeSpan(input.data<double>(), input.shape[0]);

    const auto output = cnpy::npy_load(outputNumpyFile.string());
    REQUIRE(output.word_size == 8);
    const auto outputSpan = absl::MakeSpan(output.data<double>(), output.shape[0]);
    auto size = std::min(outputSpan.size(), inputSpan.size());
    REQUIRE(size > 0);

    std::vector<Type> inputData;
    std::vector<Type> expectedData;
    inputData.reserve(size);
    expectedData.reserve(size);
    for (auto& data : inputSpan)
        inputData.push_back(static_cast<Type>(data));
    for (auto& data : outputSpan)
        expectedData.push_back(static_cast<Type>(data));

    sfz::OnePoleFilter<Type> filter { gain };
    std::vector<Type> outputData(size);
    filter.processHighpass(inputData, absl::MakeSpan(outputData));
    REQUIRE(approxEqual(outputData, expectedData));

    filter.reset();
    std::fill(outputData.begin(), outputData.end(), 0.0f);
    std::vector<Type> gains(size);
    std::fill(gains.begin(), gains.end(), gain);
    filter.processHighpassVariableGain(inputData, absl::MakeSpan(outputData), gains);
    REQUIRE(approxEqual(outputData, expectedData));
}

TEST_CASE("[OnePoleFilter] Lowpass Float")
{
    testLowpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.1.npy",
        0.1f);
    testLowpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.3.npy",
        0.3f);
    testLowpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.5.npy",
        0.5f);
    testLowpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.7.npy",
        0.7f);
    testLowpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.9.npy",
        0.9f);
}

TEST_CASE("[OnePoleFilter] Lowpass Double")
{
    testLowpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.1.npy",
        0.1f);
    testLowpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.3.npy",
        0.3f);
    testLowpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.5.npy",
        0.5f);
    testLowpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.7.npy",
        0.7f);
    testLowpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_low_gain_0.9.npy",
        0.9f);
}

TEST_CASE("[OnePoleFilter] Highpass Float")
{
    testHighpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.1.npy",
        0.1f);
    testHighpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.3.npy",
        0.3f);
    testHighpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.5.npy",
        0.5f);
    testHighpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.7.npy",
        0.7f);
    testHighpass<float>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.9.npy",
        0.9f);
}

TEST_CASE("[OnePoleFilter] Highpass Double")
{
    testHighpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.1.npy",
        0.1f);
    testHighpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.3.npy",
        0.3f);
    testHighpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.5.npy",
        0.5f);
    testHighpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.7.npy",
        0.7f);
    testHighpass<double>(
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        fs::current_path() / "tests/TestFiles/OnePoleFilter/OPF_high_gain_0.9.npy",
        0.9f);
}
