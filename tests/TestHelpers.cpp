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

const std::vector<const sfz::Voice*> getPlayingVoices(const sfz::Synth& synth)
{
    std::vector<const sfz::Voice*> playingVoices;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->releasedOrFree())
            playingVoices.push_back(voice);
    }
    return playingVoices;
}

unsigned numPlayingVoices(const sfz::Synth& synth)
{
    return absl::c_count_if(getActiveVoices(synth), [](const sfz::Voice* v) {
        return !v->releasedOrFree();
    });
}

std::string createReferenceGraph(std::vector<std::string> lines, int numRegions)
{
    for (int regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        lines.push_back(absl::StrCat(
            R"("Controller 7 {curve=4, smooth=10, value=100, step=0}" -> "Amplitude {region=)",
            regionIdx,
            R"(}")"
        ));
        lines.push_back(absl::StrCat(
            R"("Controller 10 {curve=1, smooth=10, value=100, step=0}" -> "Pan {region=)",
            regionIdx,
            R"(}")"
        ));
    }

    std::sort(lines.begin(), lines.end());

    std::string graph;
    graph.reserve(1024);

    graph += "digraph {\n";
    for (const std::string& line : lines) {
        graph.push_back('\t');
        graph += line;
        graph.push_back('\n');
    }
    graph += "}\n";

    return graph;
};
