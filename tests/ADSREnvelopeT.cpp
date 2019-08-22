#include "catch2/catch.hpp"
#include "../sources/ADSREnvelope.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <absl/types/span.h>
#include <absl/algorithm/container.h>
using namespace Catch::literals;

template<class Type>
inline bool approxEqual(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps=1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).epsilon(eps))
        {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

TEST_CASE("[ADSREnvelope] Basic state")
{
    sfz::ADSREnvelope<float> envelope;
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.0, 0.0, 0.0, 0.0, 0.0 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Attack")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 0);
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.5, 1.0, 1.0, 1.0, 1.0 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    envelope.reset(2, 0);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Attack again")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(3, 0);
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.33333, 0.66667, 1.0, 1.0, 1.0 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    envelope.reset(3, 0);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Release")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4);
    envelope.startRelease(2);
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.5, 1.0, 0.08409f, 0.00707f, 0.000594604f, 0.00005f, 0.0f, 0.0f };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );
    
    envelope.reset(2, 4);
    envelope.startRelease(2);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Delay")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 1.0f, 2);
    std::array<float, 10> output;
    envelope.startRelease(4);
    std::array<float, 10> expected { 0.0, 0.0, 0.5, 1.0, 0.08409f, 0.00707f, 0.000594604f, 0.00005f, 0.0f, 0.0f };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );
    
    envelope.reset(2, 4, 1.0f, 2);
    envelope.startRelease(4);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Lower sustain")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 0.5, 2);
    std::array<float, 10> output;
    std::array<float, 10> expected { 0.0, 0.0, 0.5, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    envelope.reset(2, 4, 0.5, 2);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Decay")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 0.5, 2, 2);
    std::array<float, 10> output;
    std::array<float, 10> expected { 0.0, 0.0, 0.5, 1.0, 0.707107, 0.5, 0.5, 0.5, 0.5, 0.5 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    envelope.reset(2, 4, 0.5, 2, 2);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Hold")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 0.5, 2, 2, 2);
    std::array<float, 12> output;
    std::array<float, 12> expected { 0.0, 0.0, 0.5, 1.0, 1.0, 1.0, 0.707107, 0.5, 0.5, 0.5, 0.5, 0.5 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );

    envelope.reset(2, 4, 0.5, 2, 2, 2);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Hold with release")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 0.5, 2, 2, 2);
    envelope.startRelease(8);
    std::array<float, 14> output;
    std::array<float, 14> expected { 0.0, 0.0, 0.5, 1.0, 1.0, 1.0, 0.707107, 0.5, 0.05, 0.005, 0.0005, 0.00005, 0.0, 0.0  };
    for (auto& out: output)
        out = envelope.getNextValue();
    
    REQUIRE( approxEqual<float>(output, expected) );
    envelope.reset(2, 4, 0.5, 2, 2, 2);
    envelope.startRelease(8);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}

TEST_CASE("[ADSREnvelope] Hold with release 2")
{
    sfz::ADSREnvelope<float> envelope;
    envelope.reset(2, 4, 0.5, 2, 2, 2);
    envelope.startRelease(4);
    std::array<float, 14> output;
    std::array<float, 14> expected { 0.0, 0.0, 0.5, 1.0, 0.08409, 0.00707, 0.000594604, 0.00005, 0.0, 0.0, 0.0, 0.0 };
    for (auto& out: output)
        out = envelope.getNextValue();
    REQUIRE( approxEqual<float>(output, expected) );
    envelope.reset(2, 4, 0.5, 2, 2, 2);
    envelope.startRelease(4);
    absl::c_fill(output, -1.0);
    envelope.getBlock(absl::MakeSpan(output));
    REQUIRE( approxEqual<float>(output, expected) );
}