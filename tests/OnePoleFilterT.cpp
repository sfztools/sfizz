#include "../sources/OnePoleFilter.h"
#include "catch2/catch.hpp"
#include "cnpy.h"
#include "gsl/gsl-lite.hpp"
#include <string>
#include <filesystem>
using namespace Catch::literals;

template<class Type>
void testInputOutput(const std::filesystem::path& inputNumpyFile, const std::filesystem::path& outputNumpyFile, Type gain)
{
    const auto input = cnpy::npy_load(inputNumpyFile.string());
    REQUIRE( input.word_size == 8 );
    const auto inputSpan = gsl::make_span(input.data<double>(), input.shape[0]);

    const auto output = cnpy::npy_load(outputNumpyFile.string());
    REQUIRE( output.word_size == 8 );
    const auto outputSpan = gsl::make_span(output.data<double>(), output.shape[0]);
    auto size = std::min(outputSpan.size(), inputSpan.size());
    REQUIRE( size > 0 );

    std::vector<Type> inputData;
    std::vector<Type> expectedData;
    inputData.reserve(size);
    expectedData.reserve(size);
    for (auto& data: inputSpan)
        inputData.push_back(static_cast<Type>(data));
    for (auto& data: outputSpan)
        expectedData.push_back(static_cast<Type>(data));
    
    OnePoleFilter filter { gain };
    std::vector<Type> outputData (size);
    filter.processLowpass(inputData, outputData);
    for (size_t i = 0; i < size; ++i)
        REQUIRE( outputData[i] == Approx(expectedData[i]) );

    filter.reset();
    std::fill(outputData.begin(), outputData.end(), 0.0);
    std::vector<Type> gains(size);
    std::fill(gains.begin(), gains.end(), gain);
    filter.processLowpassVariableGain(inputData, outputData, gains);
    for (size_t i = 0; i < size; ++i)
        REQUIRE( outputData[i] == Approx(expectedData[i]) );
}

TEST_CASE("[OnePoleFilter] Float")
{
    testInputOutput<float>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.1.npy",
        0.1f
    );
    testInputOutput<float>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.3.npy",
        0.3f
    );
    testInputOutput<float>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.5.npy",
        0.5f
    );
    testInputOutput<float>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.7.npy",
        0.7f
    );
    testInputOutput<float>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.9.npy",
        0.9f
    );
}

TEST_CASE("[OnePoleFilter] Double")
{
    testInputOutput<double>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.1.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.1.npy",
        0.1f
    );
    testInputOutput<double>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.3.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.3.npy",
        0.3f
    );
    testInputOutput<double>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.5.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.5.npy",
        0.5f
    );
    testInputOutput<double>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.7.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.7.npy",
        0.7f
    );
    testInputOutput<double>(
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_input_gain_0.9.npy",
        std::filesystem::current_path() / "tests/TestFiles/OnePoleFilter/OPF_output_gain_0.9.npy",
        0.9f
    );
}
