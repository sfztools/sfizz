// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ADSREnvelope.h"
#include "../../ADSREnvelope.h"
#include "../../Synth.h"
#include "../../Voice.h"
#include "../../Config.h"
#include "../../utility/Debug.h"

namespace sfz {

ADSREnvelopeSource::ADSREnvelopeSource(VoiceManager& manager, MidiState& state)
    : voiceManager_(manager), midiState_(state)
{
}

void ADSREnvelopeSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Voice* voice = voiceManager_.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();
    ADSREnvelope* eg = nullptr;
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

    const TriggerEvent& triggerEvent = voice->getTriggerEvent();
    const float sampleRate = voice->getSampleRate();
    eg->reset(*desc, *region, midiState_, delay, triggerEvent.value, sampleRate);
}

void ADSREnvelopeSource::release(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Voice* voice = voiceManager_.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    ADSREnvelope* eg = nullptr;

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

void ADSREnvelopeSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    Voice* voice = voiceManager_.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    ADSREnvelope* eg = nullptr;

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

    eg->getBlock(buffer);
}

} // namespace sfz
