// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "MidiState.h"
#include "Debug.h"

sfz::MidiState::MidiState()
{
    reset();
}

void sfz::MidiState::noteOnEvent(int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0 && velocity <= 127);

    if (noteNumber >= 0 && noteNumber < 128) {
        lastNoteVelocities[noteNumber] = velocity;
        noteOnTimes[noteNumber] = std::chrono::steady_clock::now();
        activeNotes++;
    }

}

void sfz::MidiState::noteOffEvent(int noteNumber, uint8_t velocity [[maybe_unused]]) noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0 && velocity <= 127);

    if (noteNumber >= 0 && noteNumber < 128) {
        if (activeNotes > 0)
            activeNotes--;
    }

}

float sfz::MidiState::getNoteDuration(int noteNumber) const
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);

    if (noteNumber >= 0 && noteNumber < 128) {
        const auto noteOffTime = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[noteNumber]);
        return duration.count();
    }

    return 0.0f;
}

uint8_t sfz::MidiState::getNoteVelocity(int noteNumber) const noexcept
{
    ASSERT(noteNumber >= 0 && noteNumber <= 127);

    return lastNoteVelocities[noteNumber];
}

void sfz::MidiState::pitchBendEvent(int pitchBendValue) noexcept
{
    ASSERT(pitchBendValue >= -8192 && pitchBendValue <= 8192);

    pitchBend = pitchBendValue;
}

int sfz::MidiState::getPitchBend() const noexcept
{
    return pitchBend;
}

void sfz::MidiState::ccEvent(int ccNumber, uint8_t ccValue) noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);
    ASSERT(ccValue >= 0 && ccValue <= 127);

    cc[ccNumber] = ccValue;
}

uint8_t sfz::MidiState::getCCValue(int ccNumber) const noexcept
{
    ASSERT(ccNumber >= 0 && ccNumber < config::numCCs);

    return cc[ccNumber];
}

const sfz::SfzCCArray& sfz::MidiState::getCCArray() const noexcept
{
    return cc;
}

void sfz::MidiState::reset() noexcept
{
    for (auto& velocity: lastNoteVelocities)
        velocity = 0;

    for (auto& ccValue: cc)
        ccValue = 0;

    pitchBend = 0;
    activeNotes = 0;
}

void sfz::MidiState::resetAllControllers() noexcept
{
    for (int idx = 0; idx < config::numCCs; idx++)
        cc[idx] = 0;

    pitchBend = 0;
}
