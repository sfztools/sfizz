// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "CommonLFO.h"
#include <cmath>

namespace sfz {
namespace fx {
namespace lfo {

    template <>
    inline float evaluateAtPhase<kTriangle>(float phase)
    {
        float y = -4 * phase + 2;
        y = (phase < 0.25f) ? (4 * phase) : y;
        y = (phase > 0.75f) ? (4 * phase - 4) : y;
        return y;
    }

    template <>
    inline float evaluateAtPhase<kSine>(float phase)
    {
        float x = phase + phase - 1;
        return 4 * x * (1 - std::fabs(x));
    }

    template <>
    inline float evaluateAtPhase<kPulse75>(float phase)
    {
        return (phase < 0.75f) ? +1.0f : -1.0f;
    }

    template <>
    inline float evaluateAtPhase<kSquare>(float phase)
    {
        return (phase < 0.5f) ? +1.0f : -1.0f;
    }

    template <>
    inline float evaluateAtPhase<kPulse25>(float phase)
    {
        return (phase < 0.25f) ? +1.0f : -1.0f;
    }

    template <>
    inline float evaluateAtPhase<kPulse12_5>(float phase)
    {
        return (phase < 0.125f) ? +1.0f : -1.0f;
    }

    template <>
    inline float evaluateAtPhase<kRamp>(float phase)
    {
        return 2 * phase - 1;
    }

    template <>
    inline float evaluateAtPhase<kSaw>(float phase)
    {
        return 1 - 2 * phase;
    }

} // namespace lfo
} // namespace fx
} // namespace sfz
