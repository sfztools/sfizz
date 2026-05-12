// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ChannelAftertouch.h"
#include "../../ModifierHelpers.h"
#include "../../ADSREnvelope.h"

namespace sfz {

ChannelAftertouchSource::ChannelAftertouchSource(VoiceManager& manager, MidiState& state)
    : midiState_(state), manager_(manager)
{
}

void ChannelAftertouchSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    UNUSED(sourceKey);
    UNUSED(voiceId);
    UNUSED(delay);
}

void ChannelAftertouchSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    UNUSED(sourceKey);
    Voice* voice = manager_.getVoiceById(voiceId);
    // MPE 1.0 §2.2.7: once released, the voice stops responding to
    // Channel Pressure on its Member Channel; Manager-Channel CP still
    // affects the release tail.
    const int channel = voice ? voice->expressionChannel() : 0;
    const EventVector& events = midiState_.getChannelAftertouchEvents(channel);
    linearEnvelope(events, buffer, [](float x) { return x; });
}

} // namespace sfz
