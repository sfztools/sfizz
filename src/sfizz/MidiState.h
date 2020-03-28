// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <array>
#include "CCMap.h"
#include "Range.h"

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
	void noteOnEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Update the state after a note off event
     *
     * @param noteNumber
     * @param velocity
     */
	void noteOffEvent(int delay, int noteNumber, float velocity) noexcept;

    int getActiveNotes() const noexcept { return activeNotes; }

    /**
     * @brief Get the note duration since note on
     *
     * @param noteNumber
     * @param delay
     * @return float
     */
	float getNoteDuration(int noteNumber, int delay = 0) const;

    /**
     * @brief Set the maximum size of the blocks for the callback. The actual
     * size can be lower in each callback but should not be larger
     * than this value.
     *
     * @param samplesPerBlock
     */
    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    /**
     * @brief Set the sample rate. If you do not call it it is initialized
     * to sfz::config::defaultSampleRate.
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate) noexcept;
    /**
     * @brief Get the note on velocity for a given note
     *
     * @param noteNumber
     * @return float
     */
	float getNoteVelocity(int noteNumber) const noexcept;

    /**
     * @brief Register a pitch bend event
     *
     * @param pitchBendValue
     */
    void pitchBendEvent(int delay, int pitchBendValue) noexcept;

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
    void ccEvent(int delay, int ccNumber, float ccValue) noexcept;

    /**
     * @brief Advances the internal clock of a given amount of samples.
     * You should call this at each callback.
     *
     * @param numSamples the number of samples of clock advance
     */
    void advanceTime(int numSamples) noexcept;
    /**
     * @brief Get the CC value for CC number
     *
     * @param ccNumber
     * @return float
     */
    float getCCValue(int ccNumber) const noexcept;

    /**
     * @brief Reset the midi state (does not impact the last note on time)
     *
     */
    void reset(int delay) noexcept;

    /**
     * @brief Reset all the controllers
     */
    void resetAllControllers(int delay) noexcept;

    /**
     * @brief Modulate a value using the last entered CCs in the midiState
     *
     * @tparam T
     * @tparam U
     * @param value the base value
     * @param modifiers the list of CC modifiers
     * @param validRange a range to clamp the output
     * @param lambda the function to apply for each modifier
     * @return T
     */
    template<class T, class U>
    T modulate(T value, const CCMap<U>& modifiers, const Range<T>& validRange, const modFunction<T, U>& lambda = addToBase<T>) const noexcept
    {
        for (auto& mod: modifiers) {
            lambda(value, getCCValue(mod.cc) * mod.value);
        }
        return validRange.clamp(value);
    }

private:
    template<class T>
    using MidiNoteArray = std::array<T, 128>;
	int activeNotes { 0 };

    /**
     * @brief Stores the note on times.
     *
     */
	MidiNoteArray<unsigned> noteOnTimes {};
    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
	MidiNoteArray<float> lastNoteVelocities;
    /**
     * @brief Current known values for the CCs.
     *
     */
	std::array<float, config::numCCs> cc;
    /**
     * Pitch bend status
     */
    int pitchBend { 0 };
    double sampleRate { config::defaultSampleRate };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    unsigned internalClock { 0 };
};
}
