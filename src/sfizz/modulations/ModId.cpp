// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModId.h"

namespace sfz {

bool ModIds::isSource(ModId id) noexcept
{
    return static_cast<int>(id) >= static_cast<int>(ModId::_SourcesStart) &&
        static_cast<int>(id) < static_cast<int>(ModId::_SourcesEnd);
}

bool ModIds::isTarget(ModId id) noexcept
{
    return static_cast<int>(id) >= static_cast<int>(ModId::_TargetsStart) &&
        static_cast<int>(id) < static_cast<int>(ModId::_TargetsEnd);
}

int ModIds::flags(ModId id) noexcept
{
    switch (id) {
        // sources
    case ModId::Controller:
        return kModIsPerCycle;
    case ModId::Envelope:
        return kModIsPerVoice;
    case ModId::LFO:
        return kModIsPerVoice;
    case ModId::AmpEG:
        return kModIsPerVoice;
    case ModId::PitchEG:
        return kModIsPerVoice;
    case ModId::FilEG:
        return kModIsPerVoice;

        // targets
    case ModId::Amplitude:
        return kModIsPerVoice|kModIsPercentMultiplicative;
    case ModId::Pan:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::Width:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::Position:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::Pitch:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::Volume:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::FilGain:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::FilCutoff:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::FilResonance:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::EqGain:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::EqFrequency:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::EqBandwidth:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::OscillatorDetune:
        return kModIsPerVoice|kModIsAdditive;
    case ModId::OscillatorModDepth:
        return kModIsPerVoice|kModIsPercentMultiplicative;

        // unknown
    default:
        return kModFlagsInvalid;
    }
}

} // namespace sfz
