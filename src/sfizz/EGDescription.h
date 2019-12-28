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
#include "Defaults.h"
#include "LeakDetector.h"
#include "SfzHelpers.h"
#include <absl/types/optional.h>


namespace sfz
{
/**
 * @brief A description for an SFZ envelope generator, with its envelope parameters
 * and possible CC modulation. This is a structure to be integrated directly in a
 * region and accessed, so not too many getters and setters in there.
 *
 * TODO: should be updated for SFZ v2
 *
 */
struct EGDescription
{
    EGDescription() = default;
    EGDescription(const EGDescription&) = default;
    EGDescription(EGDescription&&) = default;
    ~EGDescription() = default;

    float attack        { Default::attack };
    float decay         { Default::decay };
    float delay         { Default::delayEG };
    float hold          { Default::hold };
    float release       { Default::release };
    float start         { Default::start };
    float sustain       { Default::sustain };
    int   depth         { Default::depth };
    float vel2attack    { Default::attack };
    float vel2decay     { Default::decay };
    float vel2delay     { Default::delayEG };
    float vel2hold      { Default::hold };
    float vel2release   { Default::release };
    float vel2sustain   { Default::vel2sustain };
    int   vel2depth     { Default::depth };

	absl::optional<CCValuePair> ccAttack;
	absl::optional<CCValuePair> ccDecay;
	absl::optional<CCValuePair> ccDelay;
	absl::optional<CCValuePair> ccHold;
	absl::optional<CCValuePair> ccRelease;
	absl::optional<CCValuePair> ccStart;
	absl::optional<CCValuePair> ccSustain;

    /**
     * @brief Get the attack with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getAttack(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egTimeRange.clamp(ccSwitchedValue(ccValues, ccAttack, attack) + normalizeVelocity(velocity)*vel2attack);
    }
    /**
     * @brief Get the decay with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getDecay(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egTimeRange.clamp(ccSwitchedValue(ccValues, ccDecay, decay) + normalizeVelocity(velocity)*vel2decay);
    }
    /**
     * @brief Get the delay with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getDelay(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egTimeRange.clamp(ccSwitchedValue(ccValues, ccDelay, delay) + normalizeVelocity(velocity)*vel2delay);
    }
    /**
     * @brief Get the holding duration with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getHold(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egTimeRange.clamp(ccSwitchedValue(ccValues, ccHold, hold) + normalizeVelocity(velocity)*vel2hold);
    }
    /**
     * @brief Get the release duration with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getRelease(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egTimeRange.clamp(ccSwitchedValue(ccValues, ccRelease, release) + normalizeVelocity(velocity)*vel2release);
    }
    /**
     * @brief Get the starting level with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getStart(const SfzCCArray &ccValues, uint8_t velocity [[maybe_unused]]) const noexcept
    {
        return Default::egPercentRange.clamp(ccSwitchedValue(ccValues, ccStart, start));
    }
    /**
     * @brief Get the sustain level with possibly a CC modifier and a velocity modifier
     *
     * @param ccValues
     * @param velocity
     * @return float
     */
    float getSustain(const SfzCCArray &ccValues, uint8_t velocity) const noexcept
    {
        return Default::egPercentRange.clamp(ccSwitchedValue(ccValues, ccSustain, sustain) + normalizeVelocity(velocity)*vel2sustain);
    }
    LEAK_DETECTOR(EGDescription);
};

} //namespace sfz
