// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "TestHelpers.h"
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
            const sfz::ModKey::Parameters p = conn.source.parameters();
            if (p.cc == cc)
                return p;
        }
    }
    throw std::out_of_range("Region CC");
}

bool RegionCCView::match(const sfz::Region::Connection& conn) const
{
    return conn.source.id() == sfz::ModId::Controller && conn.target == target_;
}

const std::vector<const sfz::Voice*> getActiveVoices(const sfz::Synth& synth)
{
    std::vector<const sfz::Voice*> activeVoices;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->isFree())
            activeVoices.push_back(voice);
    }
    return activeVoices;
}

unsigned numPlayingVoices(const sfz::Synth& synth)
{
    return absl::c_count_if(getActiveVoices(synth), [](const sfz::Voice* v) {
        return !v->releasedOrFree();
    });
}
