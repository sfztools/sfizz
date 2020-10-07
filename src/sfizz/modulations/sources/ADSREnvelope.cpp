// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ADSREnvelope.h"
#include "../../ADSREnvelope.h"
#include "../../Synth.h"
#include "../../Voice.h"
#include "../../Config.h"
#include "../../Debug.h"

// TODO(jpc): also matrix the ampeg

namespace sfz {

ADSREnvelopeSource::ADSREnvelopeSource(Synth &synth)
    : synth_(&synth)
{
}

void ADSREnvelopeSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();
    ADSREnvelope<float>* eg = nullptr;
    const EGDescription* desc = nullptr;

    switch (sourceKey.id()) {
    case ModId::AmpEG:
        eg = voice->getAmplitudeEG();
        ASSERT(eg);
        desc = &region->amplitudeEG;
        break;
    case ModId::PitchEG:
        eg = voice->getPitchEG();
        ASSERT(eg);
        desc = &*region->pitchEG;
        break;
    case ModId::FilEG:
        eg = voice->getFilterEG();
        ASSERT(eg);
        desc = &*region->filterEG;
        break;
    default:
        ASSERTFALSE;
        return;
    }

    Resources& resources = synth.getResources();
    const TriggerEvent& triggerEvent = voice->getTriggerEvent();
    const float sampleRate = voice->getSampleRate();
    eg->reset(*desc, *region, resources.midiState, delay, triggerEvent.value, sampleRate);
}

void ADSREnvelopeSource::release(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    ADSREnvelope<float>* eg = nullptr;

    switch (sourceKey.id()) {
    case ModId::AmpEG:
        eg = voice->getAmplitudeEG();
        ASSERT(eg);
        break;
    case ModId::PitchEG:
        eg = voice->getPitchEG();
        ASSERT(eg);
        break;
    case ModId::FilEG:
        eg = voice->getFilterEG();
        ASSERT(eg);
        break;
    default:
        ASSERTFALSE;
        return;
    }

    eg->startRelease(delay);
}

ModulationSpan ADSREnvelopeSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return ModulationSpan(buffer, ModulationSpan::kInvariant);
    }

    ADSREnvelope<float>* eg = nullptr;

    switch (sourceKey.id()) {
    case ModId::AmpEG:
        eg = voice->getAmplitudeEG();
        ASSERT(eg);
        break;
    case ModId::PitchEG:
        eg = voice->getPitchEG();
        ASSERT(eg);
        break;
    case ModId::FilEG:
        eg = voice->getFilterEG();
        ASSERT(eg);
        break;
    default:
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return ModulationSpan(buffer, ModulationSpan::kInvariant);
    }

    eg->getBlock(buffer);
    return ModulationSpan(buffer);
}

} // namespace sfz
