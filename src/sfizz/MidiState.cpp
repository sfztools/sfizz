// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "MidiState.h"
#include "Macros.h"
#include "Debug.h"

sfz::MidiState::MidiState()
{
    reset();
}

void sfz::MidiState::noteOnEvent(int delay, int noteNumber, float velocity) noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0 && velocity <= 1.0);

    if (noteNumber >= 0 && noteNumber < 128) {
        lastNoteVelocities[noteNumber] = velocity;
        noteOnTimes[noteNumber] = internalClock + static_cast<unsigned>(delay);
        activeNotes++;
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
        if (activeNotes > 0)
            activeNotes--;
    }

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
    for (auto& ccEvents: cc) {
        ASSERT(!ccEvents.empty()); // CC event vectors should never be empty
        ccEvents.front().second = ccEvents.back().second;
        ccEvents.front().first = 0;
        ccEvents.resize(1);
    }
}

void sfz::MidiState::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    this->samplesPerBlock = samplesPerBlock;
}

float sfz::MidiState::getNoteDuration(int noteNumber, int delay) const
{
    ASSERT(noteNumber >= 0 && noteNumber < 128);
    if (noteNumber < 0 || noteNumber >= 128)
        return 0.0f;

    if (noteOnTimes[noteNumber] != 0 && noteOffTimes[noteNumber] != 0 && noteOnTimes[noteNumber] > noteOffTimes[noteNumber])
        return 0.0f;

    const unsigned timeInSamples = internalClock + static_cast<unsigned>(delay) - noteOnTimes[noteNumber];
    return static_cast<float>(timeInSamples) / sampleRate;
}

float sfz::MidiState::getNoteVelocity(int noteNumber) const noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);

    return lastNoteVelocities[noteNumber];
}


void sfz::MidiState::pitchBendEvent(int delay, int pitchBendValue) noexcept
{
    ASSERT(pitchBendValue >= -8192 && pitchBendValue <= 8192);

    pitchBend = pitchBendValue;
}

int sfz::MidiState::getPitchBend() const noexcept
{
    return pitchBend;
}

void sfz::MidiState::ccEvent(int delay, int ccNumber, float ccValue) noexcept
{
    ASSERT(ccValue >= 0.0 && ccValue <= 1.0);

    cc[ccNumber].emplace_back(delay, ccValue);
}

float sfz::MidiState::getCCValue(int ccNumber) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);

    return cc[ccNumber].back().second;
}

void sfz::MidiState::reset() noexcept
{
    for (auto& velocity: lastNoteVelocities)
        velocity = 0;

    for (auto& ccEvents: cc) {
        ccEvents.clear();
        ccEvents.emplace_back(0, 0.0f);
    }

    pitchBend = 0;
    activeNotes = 0;
    internalClock = 0;
    absl::c_fill(noteOnTimes, 0);
    absl::c_fill(noteOffTimes, 0);
}

void sfz::MidiState::resetAllControllers(int delay) noexcept
{
    for (unsigned ccIdx = 0; ccIdx < config::numCCs; ++ccIdx)
        ccEvent(delay, ccIdx, 0.0f);

    pitchBend = 0;
}
