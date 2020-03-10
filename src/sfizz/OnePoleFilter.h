// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "MathHelpers.h"
#include <absl/types/span.h>
#include <cmath>

namespace sfz
{
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
    // Normalized cutoff with respect to the sampling rate
    template <class C>
    static Type normalizedGain(Type cutoff, C sampleRate)
    {
        return std::tan(cutoff / static_cast<Type>(sampleRate) * fPi);
    }

    OnePoleFilter(Type gain)
    {
        setGain(gain);
    }

    void setGain(Type gain)
    {
        this->gain = gain;
        G = gain / (1 + gain);
    }

    Type getGain() const { return gain; }

    size_t processLowpass(absl::Span<const Type> input, absl::Span<Type> lowpass)
    {
        auto in = input.begin();
        auto out = lowpass.begin();
        auto size = std::min(input.size(), lowpass.size());
        auto sentinel = in + size;
        while (in < sentinel) {
            oneLowpass(in, out);
            in++;
            out++;
        }
        return size;
    }

    size_t processHighpass(absl::Span<const Type> input, absl::Span<Type> highpass)
    {
        auto in = input.begin();
        auto out = highpass.begin();
        auto size = std::min(input.size(), highpass.size());
        auto sentinel = in + size;
        while (in < sentinel) {
            oneHighpass(in, out);
            in++;
            out++;
        }
        return size;
    }

    size_t processLowpassVariableGain(absl::Span<const Type> input, absl::Span<Type> lowpass, absl::Span<const Type> gain)
    {
        auto in = input.begin();
        auto out = lowpass.begin();
        auto g = gain.begin();
        auto size = min(input.size(), lowpass.size(), gain.size());
        auto sentinel = in + size;
        while (in < sentinel) {
            setGain(*g);
            oneLowpass(in, out);
            in++;
            out++;
            g++;
        }
        return size;
    }

    size_t processHighpassVariableGain(absl::Span<const Type> input, absl::Span<Type> highpass, absl::Span<const Type> gain)
    {
        auto in = input.begin();
        auto out = highpass.begin();
        auto g = gain.begin();
        auto size = min(input.size(), highpass.size(), gain.size());
        auto sentinel = in + size;
        while (in < sentinel) {
            setGain(*g);
            oneHighpass(in, out);
            in++;
            out++;
            g++;
        }
        return size;
    }

    void reset() { state = 0.0; }

private:
    Type state { 0.0 };
    Type gain { 0.25 };
    Type intermediate { 0.0 };
    Type G { gain / (1 + gain) };

    inline void oneLowpass(const Type* in, Type* out)
    {
        intermediate = G * (*in - state);
        *out = intermediate + state;
        state = *out + intermediate;
    }

    inline void oneHighpass(const Type* in, Type* out)
    {
        intermediate = G * (*in - state);
        *out = *in - intermediate - state;
        state += 2 * intermediate;
    }
};
}
