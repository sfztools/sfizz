// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace sfz {
namespace fx {
namespace lfo {

enum Wave {
    kTriangle,
    kSine,
    kPulse75,
    kSquare,
    kPulse25,
    kPulse12_5,
    kRamp,
    kSaw,
};

template <int Wave> float evaluateAtPhase(float phase);

} // namespace lfo
} // namespace fx
} // namespace sfz

#include "CommonLFO.hpp"
