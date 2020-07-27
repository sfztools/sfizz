// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "RegionTHelpers.h"
#include "sfizz/modulations/ModId.h"

size_t RegionCCView::size() const
{
    size_t count = 0;
    for (const sfz::Region::Connection& conn : region_.connections)
        count += match(conn);
    return count;
}

bool RegionCCView::empty() const
{
    for (const sfz::Region::Connection& conn : region_.connections)
        if (match(conn))
            return false;
    return true;
}

sfz::ModKey::Parameters RegionCCView::at(int cc) const
{
    for (const sfz::Region::Connection& conn : region_.connections) {
        if (match(conn)) {
            const sfz::ModKey::Parameters p = conn.first.parameters();
            if (p.cc == cc)
                return p;
        }
    }
    throw std::out_of_range("Region CC");
}

bool RegionCCView::match(const sfz::Region::Connection& conn) const
{
    return conn.first.id() == sfz::ModId::Controller && conn.second == target_;
}
