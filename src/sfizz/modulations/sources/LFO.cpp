// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "LFO.h"
#include "../../LFO.h"
#include "../../Synth.h"
#include "../../Voice.h"
#include "../../SIMDHelpers.h"
#include "../../Config.h"
#include "../../Debug.h"

namespace sfz {

LFOSource::LFOSource(Synth &synth)
    : synth_(&synth)
{
}

void LFOSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Synth& synth = *synth_;
    unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();
    if (lfoIndex >= region->lfos.size()) {
        ASSERTFALSE;
        return;
    }

    LFO* lfo = voice->getLFO(lfoIndex);
    lfo->configure(&region->lfos[lfoIndex]);
    lfo->start(delay);
}

ModulationSpan LFOSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    Synth& synth = *synth_;
    const unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return ModulationSpan(buffer, ModulationSpan::kInvariant);
    }

    const Region* region = voice->getRegion();
    if (lfoIndex >= region->lfos.size()) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return ModulationSpan(buffer, ModulationSpan::kInvariant);
    }

    LFO* lfo = voice->getLFO(lfoIndex);
    return lfo->process(buffer);
}

} // namespace sfz
