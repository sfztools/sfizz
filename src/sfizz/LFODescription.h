// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Defaults.h"
#include <absl/types/optional.h>
#include <vector>

namespace sfz {

struct LFODescription {
    LFODescription();
    ~LFODescription();
    static const LFODescription& getDefault();
    float freq  { Default::lfoFreq }; // lfoN_freq
    float beats { Default::lfoBeats }; // lfoN_beats
    float phase0 { Default::lfoPhase }; // lfoN_phase
    float delay { Default::lfoDelay }; // lfoN_delay
    float fade { Default::lfoFade }; // lfoN_fade
    unsigned count { Default::lfoCount }; // lfoN_count
    struct Sub {
        LFOWave wave { Default::lfoWave }; // lfoN_wave[X]
        float offset { Default::lfoOffset }; // lfoN_offset[X]
        float ratio { Default::lfoRatio }; // lfoN_ratio[X]
        float scale { Default::lfoScale }; // lfoN_scale[X]
    };
    struct StepSequence {
        std::vector<float> steps {}; // lfoN_stepX - normalized to unity
    };
    absl::optional<StepSequence> seq;
    std::vector<Sub> sub;
};

} // namespace sfz
