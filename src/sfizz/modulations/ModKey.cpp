// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ModKey.h"
#include "ModId.h"
#include "../Debug.h"
#include <absl/strings/str_cat.h>
#include <cstring>

namespace sfz {

ModKey::Parameters::Parameters() noexcept
{
    // zero-fill the structure
    //  1. this ensures that non-used values will be always 0
    //  2. this makes the object memcmp-comparable
    std::memset(this, 0, sizeof(*this));
}

ModKey::Parameters::Parameters(const Parameters& other) noexcept
{
    std::memcpy(this, &other, sizeof(*this));
}

ModKey::Parameters& ModKey::Parameters::operator=(const Parameters& other) noexcept
{
    if (this != &other)
        std::memcpy(this, &other, sizeof(*this));
    return *this;
}

bool ModKey::Parameters::operator==(const Parameters& other) const noexcept
{
    return std::memcmp(this, &other, sizeof(*this)) == 0;
}

bool ModKey::Parameters::operator!=(const Parameters& other) const noexcept
{
    return std::memcmp(this, &other, sizeof(*this)) != 0;
}

ModKey ModKey::createCC(uint16_t cc, uint8_t curve, uint8_t smooth, float value, float step)
{
    ModKey::Parameters p;
    p.cc = cc;
    p.curve = curve;
    p.smooth = smooth;
    p.value = value;
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

int ModKey::flags() const noexcept
{
    return ModIds::flags(id_);
}

std::string ModKey::toString() const
{
    switch (id_) {
    case ModId::Controller:
        return absl::StrCat("Controller ", params_.cc,
            " {curve=", params_.curve, ", smooth=", params_.smooth,
            ", value=", params_.value, ", step=", params_.step, "}");
    case ModId::Envelope:
        return absl::StrCat("EG ", 1 + params_.N);
    case ModId::LFO:
        return absl::StrCat("LFO ", 1 + params_.N);

    case ModId::Amplitude:
        return "Amplitude";
    case ModId::Pan:
        return "Pan";
    case ModId::Width:
        return "Width";
    case ModId::Position:
        return "Position";
    case ModId::Pitch:
        return "Pitch";
    case ModId::Volume:
        return "Volume";

    default:
        return {};
    }
}

} // namespace sfz

bool sfz::ModKey::operator==(const ModKey &other) const noexcept
{
    return id_ == other.id_ && region_ == other.region_ &&
        parameters() == other.parameters();
}

bool sfz::ModKey::operator!=(const ModKey &other) const noexcept
{
    return !this->operator==(other);
}
