// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/ModifierHelpers.h"
#include "sfizz/Buffer.h"
#include "catch2/catch.hpp"
#include <absl/algorithm/container.h>
#include <absl/types/span.h>
#include <algorithm>
#include <array>
#include <iostream>
using namespace Catch::literals;

template <class Type>
inline bool approxEqual(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps = 1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).epsilon(eps)) {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

const auto idModifier = [](float x) { return x; };
const auto twiceModifier = [](float x) { return 2 * x; };
const auto expModifier = [](float x) { return std::exp(x); };

TEST_CASE("[Envelopes] Empty")
{
    sfz::EventVector events {
        { 0, 0.0f }
    };
    std::array<float, 5> output;
    std::array<float, 5> expected { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> expectedMul { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
    multiplicativeEnvelope(events, absl::MakeSpan(output), expModifier);
    REQUIRE(approxEqual<float>(output, expectedMul));
    multiplicativeEnvelope(events, absl::MakeSpan(output), expModifier, 2.0f);
    REQUIRE(approxEqual<float>(output, expectedMul));
}

TEST_CASE("[Envelopes] Linear basic")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 4, 1.0f }
    };
    std::array<float, 9> output;
    std::array<float, 9> expected { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] 2 events, close")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 4, 1.0f },
        { 5, 2.0f }
    };
    std::array<float, 9> output;
    std::array<float, 9> expected { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] 2 events, far")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 2.0f }
    };
    std::array<float, 9> output;
    std::array<float, 9> expected { 0.0f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.0f, 2.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] 3 events, out of block")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 2.0f },
        { 10, 3.0f }
    };
    std::array<float, 9> output;
    std::array<float, 9> expected { 0.0f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] 2 events, function")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 2.0f }
    };
    std::array<float, 9> output;
    std::array<float, 9> expected { 0.0f, 1.0f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.0f, 4.0f };
    linearEnvelope(events, absl::MakeSpan(output), twiceModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] Get quantized")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 2.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] Get quantized with unquantized targets")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.1f },
        { 6, 1.9f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] Get quantized with 2 steps")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 3.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[LinearEnvelope] Get quantized with 2 steps and an unquantized out of block step")
{
    sfz::EventVector events {
        { 0, 0.0f },
        { 2, 1.0f },
        { 6, 3.0f },
        { 10, 4.2f },
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 4.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
}


TEST_CASE("[LinearEnvelope] Going down quantized with 2 steps")
{
    sfz::EventVector events {
        { 0, 3.0f },
        { 2, 2.0f },
        { 6, 0.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 3.0f, 3.0f, 2.0f, 2.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    linearEnvelope(events, absl::MakeSpan(output), idModifier, 1.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] Basic event")
{
    sfz::EventVector events {
        { 0, 1.0f },
        { 4, 2.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.1892f, 1.4142f, 1.68176f, 2.0f, 2.0f, 2.0f, 2.0f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] 2 events")
{
    sfz::EventVector events {
        { 0, 1.0f },
        { 4, 2.0f },
        { 5, 4.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.1892f, 1.4142f, 1.68176f, 2.0f, 4.0f, 4.0f, 4.0f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] 2 events, far")
{
    sfz::EventVector events {
        { 0, 1.0f },
        { 2, 2.0f },
        { 6, 4.0f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.4142f, 2.0f, 2.37841f, 2.82843f, 3.36358f, 4.0f, 4.0f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with 2 steps")
{
    sfz::EventVector events {
        { 0, 1.3f },
        { 2, 2.1f },
        { 6, 4.2f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 4.0f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier, 2.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] Get quantized with an unquantized out of range step")
{
    sfz::EventVector events {
        { 0, 1.3f },
        { 2, 2.1f },
        { 6, 4.1f },
        { 10, 8.2f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 8.0f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier, 2.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

TEST_CASE("[MultiplicativeEnvelope] Going down quantized with 2 steps")
{
    sfz::EventVector events {
        { 0, 4.1f },
        { 2, 2.2f },
        { 6, 0.4f }
    };
    std::array<float, 8> output;
    std::array<float, 8> expected { 4.0f, 4.0f, 2.0f, 2.0f, 1.0f, 1.0f, 0.5f, 0.5f };
    multiplicativeEnvelope(events, absl::MakeSpan(output), idModifier, 2.0f);
    REQUIRE(approxEqual<float>(output, expected));
}

#if 0
TEST_CASE("[linearModifiers] Compare with envelopes")
{
    sfz::Resources resources;
    resources.curves = sfz::CurveSet::createPredefined();

    sfz::CCData<sfz::Modifier> ccData;
    ccData.cc = 20;
    ccData.data.value = 100.0f;

    resources.midiState.ccEvent(5, 20, 0.1);
    resources.midiState.ccEvent(10, 20, 0.8);

    std::array<float, 16> output;
    std::array<float, 16> envelope;

    linearEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return ccData.data.value * x;
    });
    linearModifier(resources, absl::MakeSpan(output), ccData);
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 1;
    linearEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return ccData.data.value * (2 * x - 1);
    });
    linearModifier(resources, absl::MakeSpan(output), ccData);
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 3;
    ccData.data.value = 10.0f;
    linearEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return ccData.data.value * (1 - 2 * x);
    });
    linearModifier(resources, absl::MakeSpan(output), ccData);
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 2;
    ccData.data.value = 20.0f;
    ccData.data.step = 2.5f;
    linearEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return ccData.data.value * (1 - x);
    }, ccData.data.step);
    linearModifier(resources, absl::MakeSpan(output), ccData);
    REQUIRE(approxEqual<float>(output, envelope));
}

TEST_CASE("[multiplicativeModifiers] Compare with envelopes")
{
    sfz::Resources resources;
    resources.curves = sfz::CurveSet::createPredefined();

    sfz::CCData<sfz::Modifier> ccData;
    ccData.cc = 20;
    ccData.data.value = 100.0f;

    resources.midiState.ccEvent(5, 20, 0.1);
    resources.midiState.ccEvent(15, 20, 0.8);

    std::array<float, 16> output;
    std::array<float, 16> envelope;

    multiplicativeEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return db2mag(ccData.data.value * x);
    });
    multiplicativeModifier(resources, absl::MakeSpan(output), ccData, [](float x) {
        return db2mag(x);
    });
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 1;
    multiplicativeEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return db2mag(ccData.data.value * (2 * x - 1));
    });
    multiplicativeModifier(resources, absl::MakeSpan(output), ccData, [](float x) {
        return db2mag(x);
    });
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 3;
    ccData.data.value = 10.0f;
    multiplicativeEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return db2mag(ccData.data.value * (1 - 2 * x));
    });
    multiplicativeModifier(resources, absl::MakeSpan(output), ccData, [](float x) {
        return db2mag(x);
    });
    REQUIRE(approxEqual<float>(output, envelope));

    ccData.data.curve = 2;
    ccData.data.value = 20.0f;
    ccData.data.step = 2.5f;
    multiplicativeEnvelope(resources.midiState.getCCEvents(20), absl::MakeSpan(envelope), [&ccData](float x) {
        return db2mag(ccData.data.value * (1 - x));
    }, db2mag(ccData.data.step) );
    multiplicativeModifier(resources, absl::MakeSpan(output), ccData, [](float x) {
        return db2mag(x);
    });
    REQUIRE(approxEqual<float>(output, envelope));
}
#endif
