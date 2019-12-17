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
     * @param channel (0-based)
     * @param noteNumber
     * @param velocity
     */
	void noteOnEvent(int channel, int noteNumber, uint8_t velocity) noexcept;

    /**
     * @brief Register a note off and get the note duration
     *
     * @param channel (0-based)
     * @param noteNumber
     * @return float
     */
	float getNoteDuration(int channel, int noteNumber) const;

    /**
     * @brief Get the note on velocity for a given note
     *
     * @param channel (0-based)
     * @param noteNumber
     * @return uint8_t
     */
	uint8_t getNoteVelocity(int channel, int noteNumber) const noexcept;

    /**
     * @brief Register a pitch bend event
     * 
     * @param channel 
     * @param pitchBendValue 
     */
    void pitchBendEvent(int channel, int pitchBendValue) noexcept;

    /**
     * @brief Get the pitch bend status on a channel
     * 
     * @param channel 
     * @return int 
     */
    int getPitchBend(int channel) const noexcept;

    /**
     * @brief Register a CC event
     * 
     * @param channel 
     * @param ccNumber 
     * @param ccValue 
     */
    void ccEvent(int channel, int ccNumber, uint8_t ccValue) noexcept;

    /**
     * @brief Get the CC value for a specific channel and cc number
     * 
     * @param channel 
     * @param ccNumber 
     * @return uint8_t 
     */
    uint8_t getCCValue(int channel, int ccNumber) const noexcept;

    /**
     * @brief Get the full CC status for a specific channel
     * 
     * @param channel 
     * @return const SfzCCArray& 
     */
    const SfzCCArray& getCCArray(int channel) const noexcept;

    /**
     * @brief Reset the midi state (does not impact the last note on time)
     * 
     */
    void reset() noexcept;

    /**
     * @brief Res
     * 
     * @param channel 
     */
    void resetAllControllers(int channel) noexcept;

private:
    template<class T>
    using ChannelArray = std::array<T, 16>;
    template<class T>
    using MidiNoteArray = std::array<T, 128>;
    using NoteOnTime = std::chrono::steady_clock::time_point;
    /**
     * @brief Stores the note on times.
     *
     */
	ChannelArray<MidiNoteArray<NoteOnTime>> noteOnTimes { };
    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
	ChannelArray<MidiNoteArray<uint8_t>> lastNoteVelocities { };
    /**
     * @brief Current known values for the CCs.
     *
     */
	ChannelArray<SfzCCArray> cc;
    /**
     * Pitch bend status
     */
    ChannelArray<int> pitchBends { 0 };
};
}
