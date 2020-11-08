// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Defaults.h"
#include <absl/types/optional.h>
#include <vector>

namespace sfz {

enum class LFOWave : int {
    Triangle,
    Sine,
    Pulse75,
    Square,
    Pulse25,
    Pulse12_5,
    Ramp,
    Saw,
    // ARIA extra
    RandomSH = 12,
};

struct LFODescription {
    LFODescription();
    ~LFODescription();
    static const LFODescription& getDefault();
    float freq  { Default::lfoFreq.value }; // lfoN_freq
    float beats { Default::lfoBeats.value }; // lfoN_beats
    float phase0 { Default::lfoPhase.value }; // lfoN_phase
    float delay { Default::lfoDelay.value }; // lfoN_delay
    float fade { Default::lfoFade.value }; // lfoN_fade
    unsigned count { Default::lfoCount.value }; // lfoN_count
    struct Sub {
        LFOWave wave { static_cast<LFOWave>(Default::lfoWave.value) }; // lfoN_wave[X]
        float offset { Default::lfoOffset.value }; // lfoN_offset[X]
        float ratio { Default::lfoRatio.value }; // lfoN_ratio[X]
        float scale { Default::lfoScale.value }; // lfoN_scale[X]
    };
    struct StepSequence {
        std::vector<float> steps {}; // lfoN_stepX - normalized to unity
    };
    absl::optional<StepSequence> seq;
    std::vector<Sub> sub;
};

} // namespace sfz
