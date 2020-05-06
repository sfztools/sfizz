// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Debug.h"
#include "MathHelpers.h"
#include "Macros.h"
#include <absl/types/span.h>
#include <cmath>

namespace sfz {

/**
 * @brief An implementation of a one pole filter. This is a scalar
 * implementation.
 *
 * @tparam Type the underlying type of the filter.
 */
template <class Type = float>
class OnePoleFilter {
public:
    OnePoleFilter() = default;

    void setGain(Type gain)
    {
        G = gain / (1 + gain);
    }

    void processLowpass(absl::Span<const Type> input, absl::Span<Type> output)
    {
        CHECK_SPAN_SIZES(input, output);
        processLowpass(input.data(), output.data(), minSpanSize(input, output));
    }

    void processHighpass(absl::Span<const Type> input, absl::Span<Type> output)
    {
        CHECK_SPAN_SIZES(input, output);
        processHighpass(input.data(), output.data(), minSpanSize(input, output));
    }

    void processLowpass(absl::Span<const Type> input, absl::Span<Type> output, absl::Span<const Type> gain)
    {
        CHECK_SPAN_SIZES(input, output, gain);
        processLowpass(input.data(), output.data(), gain.data(), minSpanSize(input, output, gain));
    }

    void processHighpass(absl::Span<const Type> input, absl::Span<Type> output, absl::Span<const Type> gain)
    {
        CHECK_SPAN_SIZES(input, output, gain);
        processHighpass(input.data(), output.data(), gain.data(), minSpanSize(input, output, gain));
    }

    void processLowpass(const Type* input, Type* output, unsigned size)
    {
        for (unsigned i = 0; i < size; ++i) {
            const Type intermediate = G * (input[i] - state);
            output[i] = intermediate + state;
            state = output[i] + intermediate;
        }
    }

    void processHighpass(const Type* input, Type* output, unsigned size)
    {
        for (unsigned i = 0; i < size; ++i) {
            const Type intermediate = G * (input[i] - state);
            output[i] = input[i] - intermediate - state;
            state += 2 * intermediate;
        }
    }

    void processLowpass(const Type* input, Type* output, const Type* gain, unsigned size)
    {
        for (unsigned i = 0; i < size; ++i) {
            setGain(gain[i]);
            const Type intermediate = G * (input[i] - state);
            output[i] = intermediate + state;
            state = output[i] + intermediate;
        }
    }

    void processHighpass(const Type* input, Type* output, const Type* gain, unsigned size)
    {
        for (unsigned i = 0; i < size; ++i) {
            setGain(gain[i]);
            const Type intermediate = G * (input[i] - state);
            output[i] = input[i] - intermediate - state;
            state += 2 * intermediate;
        }
    }

    void reset(Type value = 0.0)
    {
        state = value;
    }

private:
    Type state { 0.0 };
    Type G { 0.5 };
};

}
