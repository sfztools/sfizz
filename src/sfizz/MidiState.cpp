// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "MidiState.h"
#include "utility/Macros.h"
#include "utility/Debug.h"

sfz::MidiState::MidiState()
{
    resetEventStates();
    resetNoteStates();
}

void sfz::MidiState::noteOnEvent(int delay, int noteNumber, float velocity) noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0 && velocity <= 1.0);

    if (noteNumber >= 0 && noteNumber < 128) {
        float keydelta { 0 };

        if (lastNotePlayed >= 0) {
            keydelta = static_cast<float>(noteNumber - lastNotePlayed);
            velocityOverride = lastNoteVelocities[lastNotePlayed];
        }

        lastNoteVelocities[noteNumber] = velocity;
        noteOnTimes[noteNumber] = internalClock + static_cast<unsigned>(delay);
        lastNotePlayed = noteNumber;
        noteStates[noteNumber] = true;
        ccEvent(delay, ExtendedCCs::noteOnVelocity, velocity);
        ccEvent(delay, ExtendedCCs::keyboardNoteNumber, normalize7Bits(noteNumber));
        ccEvent(delay, ExtendedCCs::unipolarRandom, unipolarDist(Random::randomGenerator));
        ccEvent(delay, ExtendedCCs::bipolarRandom, bipolarDist(Random::randomGenerator));
        ccEvent(delay, ExtendedCCs::keyboardNoteGate, activeNotes > 0 ? 1.0f : 0.0f);
        ccEvent(delay, AriaExtendedCCs::keydelta, keydelta);
        ccEvent(delay, AriaExtendedCCs::absoluteKeydelta, std::abs(keydelta));
        activeNotes++;

        ccEvent(delay, ExtendedCCs::alternate, alternate);
        alternate = alternate == 0.0f ? 1.0f : 0.0f;
    }

}

void sfz::MidiState::noteOffEvent(int delay, int noteNumber, float velocity) noexcept
{
    ASSERT(delay >= 0);
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0.0 && velocity <= 1.0);
    UNUSED(velocity);
    if (noteNumber >= 0 && noteNumber < 128) {
        noteOffTimes[noteNumber] = internalClock + static_cast<unsigned>(delay);
        ccEvent(delay, ExtendedCCs::noteOffVelocity, velocity);
        ccEvent(delay, ExtendedCCs::keyboardNoteNumber, normalize7Bits(noteNumber));
        ccEvent(delay, ExtendedCCs::unipolarRandom, unipolarDist(Random::randomGenerator));
        ccEvent(delay, ExtendedCCs::bipolarRandom, bipolarDist(Random::randomGenerator));
        if (activeNotes > 0)
            activeNotes--;
        noteStates[noteNumber] = false;
    }

}

void sfz::MidiState::allNotesOff(int delay) noexcept
{
    for (int note = 0; note < 128; note++)
        noteOffEvent(delay, note, 0.0f);
}

void sfz::MidiState::setSampleRate(float sampleRate) noexcept
{
    this->sampleRate = sampleRate;
    internalClock = 0;
    absl::c_fill(noteOnTimes, 0);
    absl::c_fill(noteOffTimes, 0);
}

void sfz::MidiState::advanceTime(int numSamples) noexcept
{
    internalClock += numSamples;
    flushEvents();
}

void sfz::MidiState::flushEvents() noexcept
{
    auto flushEventVector = [] (EventVector& events) {
        if (events.empty())
            return;
        events.front().value = events.back().value;
        events.front().delay = 0;
        events.resize(1);
    };

    // Master always carries its initialised sentinel event vectors, so its
    // flush is unconditional. Member channels are populated lazily on
    // first write, so most of their event vectors stay empty under
    // non-MPE input — flushEventVector skips empties cheaply.
    for (auto& cs : channelStates) {
        for (auto& events : cs.ccEvents)
            flushEventVector(events);

        for (auto& events : cs.polyAftertouchEvents)
            flushEventVector(events);

        flushEventVector(cs.pitchEvents);
        flushEventVector(cs.channelAftertouchEvents);
    }
}


void sfz::MidiState::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    auto updateEventBufferSize = [=] (EventVector& events) {
        events.shrink_to_fit();
        events.reserve(samplesPerBlock);
    };
    this->samplesPerBlock = samplesPerBlock;
    // M1: only master channel reserves buffer space; M3 will reserve
    // for any active member channel as well.
    auto& cs = channelStates[masterChannel];
    for (auto& events: cs.ccEvents)
        updateEventBufferSize(events);

    for (auto& events: cs.polyAftertouchEvents)
        updateEventBufferSize(events);

    updateEventBufferSize(cs.pitchEvents);
    updateEventBufferSize(cs.channelAftertouchEvents);
}

float sfz::MidiState::getNoteDuration(int noteNumber, int delay) const
{
    ASSERT(noteNumber >= 0 && noteNumber < 128);
    if (noteNumber < 0 || noteNumber >= 128)
        return 0.0f;

#if 0
    if (!noteStates[noteNumber])
        return 0.0f;
#endif

    const unsigned timeInSamples = internalClock + static_cast<unsigned>(delay) - noteOnTimes[noteNumber];
    return static_cast<float>(timeInSamples) / sampleRate;
}

float sfz::MidiState::getNoteVelocity(int noteNumber) const noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);

    return lastNoteVelocities[noteNumber];
}

float sfz::MidiState::getVelocityOverride() const noexcept
{
    return velocityOverride;
}

void sfz::MidiState::insertEventInVector(EventVector& events, int delay, float value)
{
    // Member channels are populated lazily — their vectors start empty and
    // only grow on first write. linearEnvelope downstream ASSERTs the
    // vector starts at delay 0, so seed the centre-value sentinel before
    // inserting if this is the first ever event on this channel/CC slot.
    // Master channels are pre-seeded at construction so this is a no-op
    // for them.
    if (events.empty())
        events.push_back({ 0, 0.0f });

    const auto insertionPoint = absl::c_lower_bound(events, delay, MidiEventDelayComparator {});
    if (insertionPoint == events.end() || insertionPoint->delay != delay)
        events.insert(insertionPoint, { delay, value });
    else
        insertionPoint->value = value;
}

void sfz::MidiState::pitchBendEvent(int delay, float pitchBendValue) noexcept
{
    pitchBendEvent(delay, masterChannel, pitchBendValue);
}

void sfz::MidiState::pitchBendEvent(int delay, int channel, float pitchBendValue) noexcept
{
    ASSERT(pitchBendValue >= -1.0f && pitchBendValue <= 1.0f);
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return;
    insertEventInVector(channelStates[channel].pitchEvents, delay, pitchBendValue);
}

float sfz::MidiState::getPitchBend() const noexcept
{
    return getPitchBend(masterChannel);
}

float sfz::MidiState::getPitchBend(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    const auto& events = channelStates[channel].pitchEvents;
    if (events.empty()) {
        if (channel != masterChannel)
            return getPitchBend(masterChannel);
        return 0.0f;
    }
    return events.back().value;
}

void sfz::MidiState::channelAftertouchEvent(int delay, float aftertouch) noexcept
{
    channelAftertouchEvent(delay, masterChannel, aftertouch);
}

void sfz::MidiState::channelAftertouchEvent(int delay, int channel, float aftertouch) noexcept
{
    ASSERT(aftertouch >= -1.0f && aftertouch <= 1.0f);
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return;
    insertEventInVector(channelStates[channel].channelAftertouchEvents, delay, aftertouch);
}

void sfz::MidiState::polyAftertouchEvent(int delay, int noteNumber, float aftertouch) noexcept
{
    polyAftertouchEvent(delay, masterChannel, noteNumber, aftertouch);
}

void sfz::MidiState::polyAftertouchEvent(int delay, int channel, int noteNumber, float aftertouch) noexcept
{
    ASSERT(aftertouch >= 0.0f && aftertouch <= 1.0f);
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return;
    auto& events = channelStates[channel].polyAftertouchEvents;
    if (noteNumber < 0 || noteNumber >= static_cast<int>(events.size()))
        return;

    insertEventInVector(events[noteNumber], delay, aftertouch);
}

float sfz::MidiState::getChannelAftertouch() const noexcept
{
    return getChannelAftertouch(masterChannel);
}

float sfz::MidiState::getChannelAftertouch(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    const auto& events = channelStates[channel].channelAftertouchEvents;
    if (events.empty()) {
        if (channel != masterChannel)
            return getChannelAftertouch(masterChannel);
        return 0.0f;
    }
    return events.back().value;
}

float sfz::MidiState::getPolyAftertouch(int noteNumber) const noexcept
{
    return getPolyAftertouch(masterChannel, noteNumber);
}

float sfz::MidiState::getPolyAftertouch(int channel, int noteNumber) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    if (noteNumber < 0 || noteNumber > 127)
        return 0.0f;

    const auto& events = channelStates[channel].polyAftertouchEvents[noteNumber];
    if (events.empty()) {
        if (channel != masterChannel)
            return getPolyAftertouch(masterChannel, noteNumber);
        return 0.0f;
    }
    return events.back().value;
}

void sfz::MidiState::ccEvent(int delay, int ccNumber, float ccValue) noexcept
{
    ccEvent(delay, masterChannel, ccNumber, ccValue);
}

void sfz::MidiState::ccEvent(int delay, int channel, int ccNumber, float ccValue) noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return;
    if (ccNumber < 0 || ccNumber >= config::numCCs)
        return;
    insertEventInVector(channelStates[channel].ccEvents[ccNumber], delay, ccValue);
}

float sfz::MidiState::getCCValue(int ccNumber) const noexcept
{
    return getCCValue(masterChannel, ccNumber);
}

float sfz::MidiState::getCCValue(int channel, int ccNumber) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    const auto& events = channelStates[channel].ccEvents[ccNumber];
    if (events.empty()) {
        if (channel != masterChannel)
            return getCCValue(masterChannel, ccNumber);
        return 0.0f;
    }
    return events.back().value;
}

float sfz::MidiState::getCCValueAt(int ccNumber, int delay) const noexcept
{
    return getCCValueAt(masterChannel, ccNumber, delay);
}

float sfz::MidiState::getCCValueAt(int channel, int ccNumber, int delay) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    const auto& events = channelStates[channel].ccEvents[ccNumber];
    if (events.empty()) {
        if (channel != masterChannel)
            return getCCValueAt(masterChannel, ccNumber, delay);
        return 0.0f;
    }
    const auto ccEvent = absl::c_lower_bound(
        events, delay, MidiEventDelayComparator {});
    if (ccEvent != events.end())
        return ccEvent->value;
    else
        return events.back().value;
}

void sfz::MidiState::resetNoteStates() noexcept
{
    for (auto& velocity: lastNoteVelocities)
        velocity = 0.0f;

    velocityOverride = 0.0f;
    activeNotes = 0;
    internalClock = 0;
    lastNotePlayed = -1;
    alternate = 0.0f;

    auto setEvents = [] (EventVector& events, float value) {
        events.clear();
        events.push_back({ 0, value });
    };

    auto& cs = channelStates[masterChannel];
    setEvents(cs.ccEvents[ExtendedCCs::noteOnVelocity], 0.0f);
    setEvents(cs.ccEvents[ExtendedCCs::keyboardNoteNumber], 0.0f);
    setEvents(cs.ccEvents[ExtendedCCs::unipolarRandom], 0.0f);
    setEvents(cs.ccEvents[ExtendedCCs::bipolarRandom], 0.0f);
    setEvents(cs.ccEvents[ExtendedCCs::keyboardNoteGate], 0.0f);
    setEvents(cs.ccEvents[ExtendedCCs::alternate], 0.0f);

    noteStates.reset();
    absl::c_fill(noteOnTimes, 0);
    absl::c_fill(noteOffTimes, 0);
}

const sfz::EventVector& sfz::MidiState::getPitchEventsRaw(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return nullEvent;
    return channelStates[channel].pitchEvents;
}

float sfz::MidiState::getPitchBendRaw(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return 0.0f;
    const auto& events = channelStates[channel].pitchEvents;
    return events.empty() ? 0.0f : events.back().value;
}

void sfz::MidiState::setMPEPitchBendRange(float masterSemitones, float perNoteSemitones) noexcept
{
    mpeMasterPitchBendRange_ = masterSemitones;
    mpePerNotePitchBendRange_ = perNoteSemitones;
}

float sfz::MidiState::getMPEBendRangeForChannel(int channel) const noexcept
{
    return (channel == masterChannel) ? mpeMasterPitchBendRange_ : mpePerNotePitchBendRange_;
}

void sfz::MidiState::resetEventStates() noexcept
{
    auto clearEvents = [] (EventVector& events) {
        events.clear();
        events.push_back({ 0, 0.0f });
    };

    // M1: only master channel needs initialised event vectors. M3 will
    // initialise additional channels lazily on first write.
    auto& cs = channelStates[masterChannel];
    for (auto& events : cs.ccEvents)
        clearEvents(events);

    for (auto& events : cs.polyAftertouchEvents)
        clearEvents(events);

    clearEvents(cs.pitchEvents);
    clearEvents(cs.channelAftertouchEvents);
}

const sfz::EventVector& sfz::MidiState::getCCEvents(int ccIdx) const noexcept
{
    return getCCEvents(masterChannel, ccIdx);
}

const sfz::EventVector& sfz::MidiState::getCCEvents(int channel, int ccIdx) const noexcept
{
    if (ccIdx < 0 || ccIdx >= config::numCCs)
        return nullEvent;
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return nullEvent;
    const auto& events = channelStates[channel].ccEvents[ccIdx];
    if (events.empty()) {
        // MPE 1.0 inheritance: a member channel that has never received its
        // own CC value reads the master channel's value. Without this the
        // engine's defaults (CC7=Volume@~0.79, CC10=Pan@0.5, CC11=Expression
        // @1.0) collapse to 0 on member channels and voices play near-silent.
        if (channel != masterChannel)
            return channelStates[masterChannel].ccEvents[ccIdx];
        return nullEvent;
    }
    return events;
}

const sfz::EventVector& sfz::MidiState::getPitchEvents() const noexcept
{
    return getPitchEvents(masterChannel);
}

const sfz::EventVector& sfz::MidiState::getPitchEvents(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return nullEvent;
    const auto& events = channelStates[channel].pitchEvents;
    if (events.empty()) {
        if (channel != masterChannel)
            return channelStates[masterChannel].pitchEvents;
        return nullEvent;
    }
    return events;
}

const sfz::EventVector& sfz::MidiState::getChannelAftertouchEvents() const noexcept
{
    return getChannelAftertouchEvents(masterChannel);
}

const sfz::EventVector& sfz::MidiState::getChannelAftertouchEvents(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return nullEvent;
    const auto& events = channelStates[channel].channelAftertouchEvents;
    if (events.empty()) {
        if (channel != masterChannel)
            return channelStates[masterChannel].channelAftertouchEvents;
        return nullEvent;
    }
    return events;
}

const sfz::EventVector& sfz::MidiState::getPolyAftertouchEvents(int noteNumber) const noexcept
{
    return getPolyAftertouchEvents(masterChannel, noteNumber);
}

const sfz::EventVector& sfz::MidiState::getPolyAftertouchEvents(int channel, int noteNumber) const noexcept
{
    if (noteNumber < 0 || noteNumber > 127)
        return nullEvent;
    if (channel < 0 || channel >= static_cast<int>(channelStates.size()))
        return nullEvent;
    const auto& events = channelStates[channel].polyAftertouchEvents[noteNumber];
    if (events.empty()) {
        if (channel != masterChannel)
            return channelStates[masterChannel].polyAftertouchEvents[noteNumber];
        return nullEvent;
    }
    return events;
}

int sfz::MidiState::getProgram() const noexcept
{
    return currentProgram;
}

void sfz::MidiState::programChangeEvent(int delay, int program) noexcept
{
    UNUSED(delay);
    ASSERT(program >= 0 && program <= 127);
    currentProgram = program;
}
