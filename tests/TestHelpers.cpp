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

float RegionCCView::valueAt(int cc) const
{
    for (const sfz::Region::Connection& conn : region_.connections) {
        if (match(conn)) {
            const sfz::ModKey::Parameters p = conn.source.parameters();
            if (p.cc == cc)
                return conn.sourceDepth;
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

const std::vector<std::string> playingSamples(const sfz::Synth& synth)
{
    std::vector<std::string> samples;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->releasedOrFree()) {
            if (auto region = voice->getRegion())
                samples.push_back(region->sampleId->filename());
        }
    }
    return samples;
}

const std::vector<float> playingVelocities(const sfz::Synth& synth)
{
    std::vector<float> velocities;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->releasedOrFree())
            velocities.push_back(voice->getTriggerEvent().value);
    }
    return velocities;
}

const std::vector<std::string> activeSamples(const sfz::Synth& synth)
{
    std::vector<std::string> samples;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->isFree()) {
            const sfz::Region* region = voice->getRegion();
            if (region)
                samples.push_back(region->sampleId->filename());
        }
    }
    return samples;
}

const std::vector<float> activeVelocities(const sfz::Synth& synth)
{
    std::vector<float> velocities;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        const auto* voice = synth.getVoiceView(i);
        if (!voice->isFree())
            velocities.push_back(voice->getTriggerEvent().value);
    }
    return velocities;
}

std::string createDefaultGraph(std::vector<std::string> lines, int numRegions)
{
    for (int regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        lines.push_back(absl::StrCat(
            R"("AmplitudeEG {)", regionIdx, R"(}" -> "MasterAmplitude {)", regionIdx, R"(}")"
        ));
        lines.push_back(absl::StrCat(
            R"("Controller 7 {curve=4, smooth=10, step=0}" -> "Amplitude {)",
            regionIdx,
            R"(}")"
        ));
        lines.push_back(absl::StrCat(
            R"("Controller 10 {curve=1, smooth=10, step=0}" -> "Pan {)",
            regionIdx,
            R"(}")"
        ));
        lines.push_back(absl::StrCat(
            R"("Controller 11 {curve=4, smooth=10, step=0}" -> "Amplitude {)",
            regionIdx,
            R"(}")"
        ));
    }

    return createModulationDotGraph(lines);
};

std::string createModulationDotGraph(std::vector<std::string> lines)
{
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
}

void simpleMessageReceiver(void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    (void)delay;
    auto& messageList = *reinterpret_cast<std::vector<std::string>*>(data);

    std::string newMessage = absl::StrCat(path, ",", sig, " : { ");
    for (unsigned i = 0, n = strlen(sig); i < n; ++i) {
        switch(sig[i]){
        case 'i':
            absl::StrAppend(&newMessage, args[i].i);
            break;
        case 'f':
            absl::StrAppend(&newMessage, args[i].f);
            break;
        case 'd':
            absl::StrAppend(&newMessage, args[i].d);
            break;
        case 'h':
            absl::StrAppend(&newMessage, args[i].h);
            break;
        case 's':
            absl::StrAppend(&newMessage, args[i].s);
            break;
        }

        if (i == (n - 1))
            absl::StrAppend(&newMessage, " }");
        else
            absl::StrAppend(&newMessage, ", ");
    }

    messageList.push_back(std::move(newMessage));
}
