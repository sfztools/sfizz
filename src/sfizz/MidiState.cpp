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
        ASSERT(!events.empty()); // CC event vectors should never be empty
        events.front().value = events.back().value;
        events.front().delay = 0;
        events.resize(1);
    };

    // M1: only master channel is populated; M3 will iterate over all
    // channels that received events this block.
    auto& cs = channelStates[masterChannel];
    for (auto& events : cs.ccEvents)
        flushEventVector(events);

    for (auto& events: cs.polyAftertouchEvents)
        flushEventVector(events);

    flushEventVector(cs.pitchEvents);
    flushEventVector(cs.channelAftertouchEvents);
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
    const auto insertionPoint = absl::c_lower_bound(events, delay, MidiEventDelayComparator {});
    if (insertionPoint == events.end() || insertionPoint->delay != delay)
        events.insert(insertionPoint, { delay, value });
    else
        insertionPoint->value = value;
}

void sfz::MidiState::pitchBendEvent(int delay, float pitchBendValue) noexcept
{
    ASSERT(pitchBendValue >= -1.0f && pitchBendValue <= 1.0f);
    insertEventInVector(channelStates[masterChannel].pitchEvents, delay, pitchBendValue);
}

float sfz::MidiState::getPitchBend() const noexcept
{
    const auto& events = channelStates[masterChannel].pitchEvents;
    ASSERT(events.size() > 0);
    return events.back().value;
}

void sfz::MidiState::channelAftertouchEvent(int delay, float aftertouch) noexcept
{
    ASSERT(aftertouch >= -1.0f && aftertouch <= 1.0f);
    insertEventInVector(channelStates[masterChannel].channelAftertouchEvents, delay, aftertouch);
}

void sfz::MidiState::polyAftertouchEvent(int delay, int noteNumber, float aftertouch) noexcept
{
    ASSERT(aftertouch >= 0.0f && aftertouch <= 1.0f);
    auto& events = channelStates[masterChannel].polyAftertouchEvents;
    if (noteNumber < 0 || noteNumber >= static_cast<int>(events.size()))
        return;

    insertEventInVector(events[noteNumber], delay, aftertouch);
}

float sfz::MidiState::getChannelAftertouch() const noexcept
{
    const auto& events = channelStates[masterChannel].channelAftertouchEvents;
    ASSERT(events.size() > 0);
    return events.back().value;
}

float sfz::MidiState::getPolyAftertouch(int noteNumber) const noexcept
{
    if (noteNumber < 0 || noteNumber > 127)
        return 0.0f;

    const auto& events = channelStates[masterChannel].polyAftertouchEvents[noteNumber];
    ASSERT(events.size() > 0);
    return events.back().value;
}

void sfz::MidiState::ccEvent(int delay, int ccNumber, float ccValue) noexcept
{
    insertEventInVector(channelStates[masterChannel].ccEvents[ccNumber], delay, ccValue);
}

float sfz::MidiState::getCCValue(int ccNumber) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);
    return channelStates[masterChannel].ccEvents[ccNumber].back().value;
}

float sfz::MidiState::getCCValueAt(int ccNumber, int delay) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);
    const auto& events = channelStates[masterChannel].ccEvents[ccNumber];
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
    if (ccIdx < 0 || ccIdx >= config::numCCs)
        return nullEvent;

    return channelStates[masterChannel].ccEvents[ccIdx];
}

const sfz::EventVector& sfz::MidiState::getPitchEvents() const noexcept
{
    return channelStates[masterChannel].pitchEvents;
}

const sfz::EventVector& sfz::MidiState::getChannelAftertouchEvents() const noexcept
{
    return channelStates[masterChannel].channelAftertouchEvents;
}

const sfz::EventVector& sfz::MidiState::getPolyAftertouchEvents(int noteNumber) const noexcept
{
    if (noteNumber < 0 || noteNumber > 127)
        return nullEvent;

    return channelStates[masterChannel].polyAftertouchEvents[noteNumber];
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
