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

#pragma once
#include "Config.h"
#include "MathHelpers.h"
#include <absl/types/span.h>
#include <cmath>

template <class Type = float>
class OnePoleFilter {
public:
    OnePoleFilter() = default;
    // Normalized cutoff with respect to the sampling rate
    template <class C>
    static Type normalizedGain(Type cutoff, C sampleRate)
    {
        return std::tan(cutoff / static_cast<Type>(sampleRate) * pi<float>);
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

    int processLowpass(absl::Span<const Type> input, absl::Span<Type> lowpass)
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

    int processHighpass(absl::Span<const Type> input, absl::Span<Type> highpass)
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

    int processLowpassVariableGain(absl::Span<const Type> input, absl::Span<Type> lowpass, absl::Span<const Type> gain)
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

    int processHighpassVariableGain(absl::Span<const Type> input, absl::Span<Type> highpass, absl::Span<const Type> gain)
    {
        auto in = input.begin();
        auto out = highpass.begin();
        auto g = gain.begin();
        auto size = min(input.size(), highpass.size(), gain.size());
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