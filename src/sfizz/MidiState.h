#pragma once
#include <chrono>
#include <array>
#include "SfzHelpers.h"

namespace sfz
{
/**
 * @brief Holds the current "MIDI state", meaning the known state of all CCs
 * currently, as well as the note velocities that triggered the currently
 * pressed notes.
 *
 */
struct MidiState
{
    /**
     * @brief Update the state after a note on event
     *
     * @param noteNumber
     * @param velocity
     */
	inline void noteOn(int noteNumber, uint8_t velocity)
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			lastNoteVelocities[noteNumber] = velocity;
			noteOnTimes[noteNumber] = std::chrono::steady_clock::now();
		}
	}

    /**
     * @brief Register a note off and get the note duration
     *
     * @param noteNumber
     * @return float
     */
	inline float getNoteDuration(int noteNumber) const
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			const auto noteOffTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[noteNumber]);
			return duration.count();
		}

		return 0.0f;
	}

    /**
     * @brief Get the note on velocity for a given note
     *
     * @param noteNumber
     * @return uint8_t
     */
	inline uint8_t getNoteVelocity(int noteNumber) const
	{
		if (noteNumber >= 0 && noteNumber < 128)
			return lastNoteVelocities[noteNumber];

		return 0;
	}

    inline void pitchBendEvent(int pitchBendValue)
    {
        pitchBend = pitchBendValue;
    }

    /**
     * @brief Stores the note on times.
     *
     */
	std::array<std::chrono::steady_clock::time_point, 128> noteOnTimes { };
    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
	std::array<uint8_t, 128> lastNoteVelocities { };
    /**
     * @brief Current known values for the CCs.
     *
     */
	CCValueArray cc;
    /**
     * Pitch bend status
     */
    int pitchBend { 0 };
};
}
