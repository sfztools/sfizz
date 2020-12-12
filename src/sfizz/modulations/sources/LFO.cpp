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

LFOSource::LFOSource(VoiceManager& manager)
    : voiceManager_(manager)
{
}

void LFOSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = voiceManager_.getVoiceById(voiceId);
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

void LFOSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    const unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = voiceManager_.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return;
    }

    const Region* region = voice->getRegion();
    if (lfoIndex >= region->lfos.size()) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return;
    }

    LFO* lfo = voice->getLFO(lfoIndex);
    lfo->process(buffer, region->getId());
}

} // namespace sfz
