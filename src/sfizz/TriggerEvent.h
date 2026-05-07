// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace sfz
{
enum class TriggerEventType { NoteOn, NoteOff, CC };

/**
 * @brief Encapsulate a midi event with normalized values
 *
 */
struct TriggerEvent
{
    TriggerEventType type;
    int number;
    float value;
    /**
     * @brief MIDI channel (0..15) the event was sent on. Defaults to 0
     * (master), so existing call sites that brace-initialize with
     * three fields keep their previous behavior. The value flows down
     * to the triggered Voice so per-voice modulation reads can route
     * to the correct slot in MidiState's per-channel storage.
     */
    int channel { 0 };
};

}
