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
#include <optional>


namespace sfz
{

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

    std::optional<CCValuePair> ccAttack;
    std::optional<CCValuePair> ccDecay;
    std::optional<CCValuePair> ccDelay;
    std::optional<CCValuePair> ccHold;
    std::optional<CCValuePair> ccRelease;
    std::optional<CCValuePair> ccStart;
    std::optional<CCValuePair> ccSustain;

    float getAttack(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccAttack, attack) + normalizeCC(velocity)*vel2attack;
    }
    float getDecay(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccDecay, decay) + normalizeCC(velocity)*vel2decay;
    }
    float getDelay(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccDelay, delay) + normalizeCC(velocity)*vel2delay;
    }
    float getHold(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccHold, hold) + normalizeCC(velocity)*vel2hold;
    }
    float getRelease(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccRelease, release) + normalizeCC(velocity)*vel2release;
    }
    float getStart(const CCValueArray &ccValues, uint8_t velocity [[maybe_unused]]) const noexcept
    {
        return ccSwitchedValue(ccValues, ccStart, start);
    }
    float getSustain(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccSustain, sustain) + normalizeCC(velocity)*vel2sustain;
    }
    LEAK_DETECTOR(EGDescription);
};

} //namespace sfz