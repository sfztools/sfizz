// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModKey.h"
#include "../Debug.h"
#include <absl/strings/str_cat.h>

namespace sfz {

ModKey::Parameters::Parameters() noexcept
{
    // zero-fill the structure
    //  1. this ensures that non-used values will be always 0
    //  2. this makes the object memcmp-comparable
    std::memset(
        static_cast<RawParameters*>(this),
        0, sizeof(RawParameters));
}

ModKey::Parameters::Parameters(const Parameters& other) noexcept
{
    std::memcpy(
        static_cast<RawParameters*>(this),
        static_cast<const RawParameters*>(&other),
        sizeof(RawParameters));
}

ModKey::Parameters& ModKey::Parameters::operator=(const Parameters& other) noexcept
{
    if (this != &other)
        std::memcpy(
            static_cast<RawParameters*>(this),
            static_cast<const RawParameters*>(&other),
            sizeof(RawParameters));
    return *this;
}

ModKey::Parameters::Parameters(Parameters&& other) noexcept
{
    std::memcpy(
        static_cast<RawParameters*>(this),
        static_cast<const RawParameters*>(&other),
        sizeof(RawParameters));
}

ModKey::Parameters& ModKey::Parameters::operator=(Parameters&& other) noexcept
{
    if (this != &other)
        std::memcpy(
            static_cast<RawParameters*>(this),
            static_cast<const RawParameters*>(&other),
            sizeof(RawParameters));
    return *this;
}

ModKey ModKey::createCC(uint16_t cc, uint8_t curve, uint8_t smooth, float step)
{
    ModKey::Parameters p;
    p.cc = cc;
    p.curve = curve;
    p.smooth = smooth;
    p.step = step;
    return ModKey(ModId::Controller, {}, p);
}

ModKey ModKey::createNXYZ(ModId id, NumericId<Region> region, uint8_t N, uint8_t X, uint8_t Y, uint8_t Z)
{
    ASSERT(id != ModId::Controller);
    ModKey::Parameters p;
    p.N = N;
    p.X = X;
    p.Y = Y;
    p.Z = Z;
    return ModKey(id, region, p);
}

bool ModKey::isSource() const noexcept
{
    return ModIds::isSource(id_);
}

bool ModKey::isTarget() const noexcept
{
    return ModIds::isTarget(id_);
}


std::string ModKey::toString() const
{
    switch (id_) {
    case ModId::Controller:
        return absl::StrCat("Controller ", params_.cc,
            " {curve=", params_.curve, ", smooth=", params_.smooth,
            ", step=", params_.step, "}");
    case ModId::Envelope:
        return absl::StrCat("EG ", 1 + params_.N, " {", region_.number(), "}");
    case ModId::LFO:
        return absl::StrCat("LFO ", 1 + params_.N, " {", region_.number(), "}");
    case ModId::AmpLFO:
        return absl::StrCat("AmplitudeLFO {", region_.number(), "}");
    case ModId::PitchLFO:
        return absl::StrCat("PitchLFO {", region_.number(), "}");
    case ModId::FilLFO:
        return absl::StrCat("FilterLFO {", region_.number(), "}");
    case ModId::AmpEG:
        return absl::StrCat("AmplitudeEG {", region_.number(), "}");
    case ModId::PitchEG:
        return absl::StrCat("PitchEG {", region_.number(), "}");
    case ModId::FilEG:
        return absl::StrCat("FilterEG {", region_.number(), "}");
    case ModId::ChannelAftertouch:
        return absl::StrCat("ChannelAftertouch");

    case ModId::MasterAmplitude:
        return absl::StrCat("MasterAmplitude {", region_.number(), "}");
    case ModId::Amplitude:
        return absl::StrCat("Amplitude {", region_.number(), "}");
    case ModId::Pan:
        return absl::StrCat("Pan {", region_.number(), "}");
    case ModId::Width:
        return absl::StrCat("Width {", region_.number(), "}");
    case ModId::Position:
        return absl::StrCat("Position {", region_.number(), "}");
    case ModId::Pitch:
        return absl::StrCat("Pitch {", region_.number(), "}");
    case ModId::Volume:
        return absl::StrCat("Volume {", region_.number(), "}");
    case ModId::FilGain:
        return absl::StrCat("FilterGain {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::FilCutoff:
        return absl::StrCat("FilterCutoff {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::FilResonance:
        return absl::StrCat("FilterResonance {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::EqGain:
        return absl::StrCat("EqGain {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::EqFrequency:
        return absl::StrCat("EqFrequency {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::EqBandwidth:
        return absl::StrCat("EqBandwidth {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::OscillatorDetune:
        return absl::StrCat("OscillatorDetune {", region_.number(), ", N=", 1 + params_.N, "}");
   case ModId::OscillatorModDepth:
        return absl::StrCat("OscillatorModDepth {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::LFOFrequency:
        return absl::StrCat("LFOFrequency {", region_.number(), ", N=", 1 + params_.N, "}");
    case ModId::LFOBeats:
        return absl::StrCat("LFOBeats {", region_.number(), ", N=", 1 + params_.N, "}");

    default:
        return {};
    }
}

} // namespace sfz

