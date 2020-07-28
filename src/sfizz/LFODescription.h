// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
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
    float freq = 0; // lfoN_freq
    float phase0 = 0; // lfoN_phase
    float delay = 0; // lfoN_delay
    float fade = 0; // lfoN_fade
    unsigned count = 0; // lfoN_count
    struct Sub {
        LFOWave wave = LFOWave::Triangle; // lfoN_wave[X]
        float offset = 0; // lfoN_offset[X]
        float ratio = 1; // lfoN_ratio[X]
        float scale = 1; // lfoN_scale[X]
    };
    struct StepSequence {
        std::vector<float> steps {}; // lfoN_stepX - normalized to unity
    };
    absl::optional<StepSequence> seq;
    std::vector<Sub> sub;
};

} // namespace sfz
