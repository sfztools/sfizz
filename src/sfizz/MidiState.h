// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <array>
#include <bitset>
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
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOnEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Update the state after a note off event
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOffEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Set all notes off
     *
     * @param delay
     */
    void allNotesOff(int delay) noexcept;

    /**
     * @brief Get the number of active notes
     */
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
     * @brief Get the velocity of the last note played
     *
     * @return float
     */
    float getLastVelocity() const noexcept;

    /**
     * @brief Register a pitch bend event
     *
     * @param pitchBendValue
     */
    void pitchBendEvent(int delay, float pitchBendValue) noexcept;

    /**
     * @brief Get the pitch bend status

     * @return int
     */
    float getPitchBend() const noexcept;

    /**
     * @brief Register a channel aftertouch event
     *
     * @param aftertouch
     */
    void channelAftertouchEvent(int delay, float aftertouch) noexcept;

    /**
     * @brief Register a channel aftertouch event
     *
     * @param aftertouch
     */
    void polyAftertouchEvent(int delay, int noteNumber, float aftertouch) noexcept;

    /**
     * @brief Get the channel aftertouch status

     * @return int
     */
    float getChannelAftertouch() const noexcept;

    /**
     * @brief Get the polyphonic aftertouch status

     * @return int
     */
    float getPolyAftertouch(int noteNumber) const noexcept;

    /**
     * @brief Register a CC event
     *
     * @param ccNumber
     * @param ccValue
     */
    void ccEvent(int delay, int ccNumber, float ccValue) noexcept;

    /**
     * @brief Advances the internal clock of a given amount of samples.
     * You should call this at each callback. This will flush the events
     * in the midistate memory by calling flushEvents().
     *
     * @param numSamples the number of samples of clock advance
     */
    void advanceTime(int numSamples) noexcept;

    /**
     * @brief Flush events in all states, keeping only the last one as the "base" state
     *
     */
    void flushEvents() noexcept;

    /**
     * @brief Check if a note is currently depressed
     *
     * @param noteNumber
     * @return true
     * @return false
     */
    bool isNotePressed(int noteNumber) const noexcept { return noteStates[noteNumber]; }

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
    void reset() noexcept;

    /**
     * @brief Reset all the controllers
     */
    void resetAllControllers(int delay) noexcept;

    const EventVector& getCCEvents(int ccIdx) const noexcept;
    const EventVector& getPolyAftertouchEvents(int noteNumber) const noexcept;
    const EventVector& getPitchEvents() const noexcept;
    const EventVector& getChannelAftertouchEvents() const noexcept;

    /**
     * @brief Get the alternate state value, for extended CC 137
     *
     * @return float
     */
    float getAlternateState() const noexcept { return alternate; }

private:

    /**
     * @brief Insert events in a sorted event vector.
     *
     * @param events
     * @param delay
     * @param value
     */
    void insertEventInVector(EventVector& events, int delay, float value);

    int activeNotes { 0 };

    /**
     * @brief Stores the note on times.
     *
     */
    MidiNoteArray<unsigned> noteOnTimes { {} };

    /**
     * @brief Stores the note off times.
     *
     */

    MidiNoteArray<unsigned> noteOffTimes { {} };

    /**
     * @brief Store the note states
     *
     */
    std::bitset<128> noteStates;

    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
    MidiNoteArray<float> lastNoteVelocities;

    /**
     * @brief Last note played
     */
    int lastNotePlayed { 0 };

    /**
     * @brief Current known values for the CCs.
     *
     */
    std::array<EventVector, config::numCCs> ccEvents;

    /**
     * @brief Null event
     *
     */
    const EventVector nullEvent { { 0, 0.0f } };

    /**
     * @brief Pitch bend status
     */
    EventVector pitchEvents;

    /**
     * @brief Aftertouch status
     */
    EventVector channelAftertouchEvents;

    /**
     * @brief Polyphonic aftertouch status.
     */
    std::array<EventVector, 128> polyAftertouchEvents;

    float sampleRate { config::defaultSampleRate };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    float alternate { 0.0f };
    unsigned internalClock { 0 };
};
}
