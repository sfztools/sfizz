// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
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
#include "Defaults.h"
#include "Macros.h"
#include "LeakDetector.h"
#include "SfzHelpers.h"
#include "MidiState.h"
#include <absl/types/optional.h>

namespace sfz {
/**
 * @brief A description for an SFZ envelope generator, with its envelope parameters
 * and possible CC modulation. This is a structure to be integrated directly in a
 * region and accessed, so not too many getters and setters in there.
 *
 * TODO: should be updated for SFZ v2
 *
 */

/**
 * @brief If a cc switch exists for the value, returns the value with the CC modifier, otherwise returns the value alone.
 *
 * @param ccValues
 * @param ccSwitch
 * @param value
 * @return float
 */
inline float ccSwitchedValue(const MidiState& state, const absl::optional<CCData<float>>& ccSwitch, float value) noexcept
{
    if (ccSwitch)
        return value + ccSwitch->data * state.getCCValue(ccSwitch->cc);
    else
        return value;
}

struct EGDescription {
    EGDescription() = default;
    EGDescription(const EGDescription&) = default;
    EGDescription(EGDescription&&) = default;
    ~EGDescription() = default;

    float attack { Default::attack };
    float decay { Default::decay };
    float delay { Default::delayEG };
    float hold { Default::hold };
    float release { Default::release };
    float start { Default::start };
    float sustain { Default::sustain };
    int depth { Default::depth };
    float vel2attack { Default::attack };
    float vel2decay { Default::decay };
    float vel2delay { Default::delayEG };
    float vel2hold { Default::hold };
    float vel2release { Default::vel2release };
    float vel2sustain { Default::vel2sustain };
    int vel2depth { Default::depth };

    absl::optional<CCData<float>> ccAttack;
    absl::optional<CCData<float>> ccDecay;
    absl::optional<CCData<float>> ccDelay;
    absl::optional<CCData<float>> ccHold;
    absl::optional<CCData<float>> ccRelease;
    absl::optional<CCData<float>> ccStart;
    absl::optional<CCData<float>> ccSustain;

    /**
     * @brief Get the attack with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getAttack(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egTimeRange.clamp(ccSwitchedValue(state, ccAttack, attack) + velocity * vel2attack);
    }
    /**
     * @brief Get the decay with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getDecay(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egTimeRange.clamp(ccSwitchedValue(state, ccDecay, decay) + velocity * vel2decay);
    }
    /**
     * @brief Get the delay with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getDelay(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egTimeRange.clamp(ccSwitchedValue(state, ccDelay, delay) + velocity * vel2delay);
    }
    /**
     * @brief Get the holding duration with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getHold(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egTimeRange.clamp(ccSwitchedValue(state, ccHold, hold) + velocity * vel2hold);
    }
    /**
     * @brief Get the release duration with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getRelease(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egTimeRange.clamp(ccSwitchedValue(state, ccRelease, release) + velocity * vel2release);
    }
    /**
     * @brief Get the starting level with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getStart(const MidiState& state, float velocity) const noexcept
    {
        UNUSED(velocity);
        return Default::egPercentRange.clamp(ccSwitchedValue(state, ccStart, start));
    }
    /**
     * @brief Get the sustain level with possibly a CC modifier and a velocity modifier
     *
     * @param state
     * @param velocity
     * @return float
     */
    float getSustain(const MidiState& state, float velocity) const noexcept
    {
        ASSERT(velocity >= 0.0f && velocity <= 1.0f);
        return Default::egPercentRange.clamp(ccSwitchedValue(state, ccSustain, sustain) + velocity * vel2sustain);
    }
    LEAK_DETECTOR(EGDescription);
};

} //namespace sfz
