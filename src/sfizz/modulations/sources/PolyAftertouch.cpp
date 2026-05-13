// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PolyAftertouch.h"
#include "../../ModifierHelpers.h"
#include "../../ADSREnvelope.h"

namespace sfz {

PolyAftertouchSource::PolyAftertouchSource(VoiceManager& manager, MidiState& state)
    : midiState_(state), manager_(manager)
{

}

void PolyAftertouchSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    UNUSED(sourceKey);
    UNUSED(voiceId);
    UNUSED(delay);
}

void PolyAftertouchSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    UNUSED(sourceKey);
    Voice* voice = manager_.getVoiceById(voiceId);
    if (!voice || voice->getTriggerEvent().type == TriggerEventType::CC) {
        fill(buffer, 0.0f);
        return;
    }

    const int noteNumber = voice->getTriggerEvent().number;
    // Defense-in-depth: MPE 1.0 §2.2.7 prohibits Poly Key Pressure on
    // Member Channels (Synth::hdPolyAftertouch drops it before it
    // reaches MidiState), and once released a voice should not be reading
    // its old Member Channel anyway since the controller will have
    // reallocated it. Route through the released-note channel gate so
    // both rules hold even if the engine later relaxes one.
    const int channel = voice->expressionChannel();

    const EventVector& events = midiState_.getPolyAftertouchEvents(channel, noteNumber);
    linearEnvelope(events, buffer, [](float x) { return x; });
}

} // namespace sfz
