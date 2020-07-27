// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz/Region.h"
#include "sfizz/modulations/ModKey.h"

class RegionCCView {
public:
    RegionCCView(const sfz::Region& region, sfz::ModKey target)
        : region_(region), target_(target)
    {
    }

    size_t size() const;
    bool empty() const;
    sfz::ModKey::Parameters at(int cc) const;

private:
    bool match(const sfz::Region::Connection& conn) const;

private:
    const sfz::Region& region_;
    sfz::ModKey target_;
};
