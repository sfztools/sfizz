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
class MidiState
{
public:
    MidiState();
    /**
     * @brief Update the state after a note on event
     *
     * @param noteNumber
     * @param velocity
     */
	void noteOnEvent(int noteNumber, uint8_t velocity) noexcept;

    /**
     * @brief Update the state after a note off event
     *
     * @param noteNumber
     * @param velocity
     */
	void noteOffEvent(int noteNumber, uint8_t velocity) noexcept;

    int getActiveNotes() const noexcept { return activeNotes; }

    /**
     * @brief Register a note off and get the note duration
     *
     * @param noteNumber
     * @return float
     */
	float getNoteDuration(int noteNumber) const;

    /**
     * @brief Get the note on velocity for a given note
     *
     * @param noteNumber
     * @return uint8_t
     */
	uint8_t getNoteVelocity(int noteNumber) const noexcept;

    /**
     * @brief Register a pitch bend event
     *
     * @param pitchBendValue
     */
    void pitchBendEvent(int pitchBendValue) noexcept;

    /**
     * @brief Get the pitch bend status

     * @return int
     */
    int getPitchBend() const noexcept;

    /**
     * @brief Register a CC event
     *
     * @param ccNumber
     * @param ccValue
     */
    void ccEvent(int ccNumber, uint8_t ccValue) noexcept;

    /**
     * @brief Get the CC value for CC number
     *
     * @param ccNumber
     * @return uint8_t
     */
    uint8_t getCCValue(int ccNumber) const noexcept;

    /**
     * @brief Get the full CC status
     *
     * @return const SfzCCArray&
     */
    const SfzCCArray& getCCArray() const noexcept;

    /**
     * @brief Reset the midi state (does not impact the last note on time)
     *
     */
    void reset() noexcept;

    /**
     * @brief Reset all the controllers
     */
    void resetAllControllers() noexcept;

private:
    template<class T>
    using MidiNoteArray = std::array<T, 128>;
    using NoteOnTime = std::chrono::steady_clock::time_point;
	int activeNotes { 0 };
    /**
     * @brief Stores the note on times.
     *
     */
	MidiNoteArray<NoteOnTime> noteOnTimes { };
    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
	MidiNoteArray<uint8_t> lastNoteVelocities { };
    /**
     * @brief Current known values for the CCs.
     *
     */
	SfzCCArray cc;
    /**
     * Pitch bend status
     */
    int pitchBend { 0 };
};
}
