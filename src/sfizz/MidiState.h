#pragma once
#include <chrono>
#include <array>
#include "SfzHelpers.h"
#include "Debug.h"

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
    MidiState() noexcept
    {
        reset();
    }
    /**
     * @brief Update the state after a note on event
     *
     * @param channel (1-based)
     * @param noteNumber
     * @param velocity
     */
	void noteOnEvent(int channel, int noteNumber, uint8_t velocity) noexcept
	{
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(noteNumber >= 0 && noteNumber <= 127);
        ASSERT(velocity >= 0 && velocity <= 127);
        channel = translateSfzChannelToMidi(channel);
		if (noteNumber >= 0 && noteNumber < 128) {
			lastNoteVelocities[channel][noteNumber] = velocity;
			noteOnTimes[channel][noteNumber] = std::chrono::steady_clock::now();
		}
	}

    /**
     * @brief Register a note off and get the note duration
     *
     * @param channel (1-based)
     * @param noteNumber
     * @return float
     */
	float getNoteDuration(int channel, int noteNumber) const
	{
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(noteNumber >= 0 && noteNumber <= 127);
        channel = translateSfzChannelToMidi(channel);
		if (noteNumber >= 0 && noteNumber < 128) {
			const auto noteOffTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[channel][noteNumber]);
			return duration.count();
		}

		return 0.0f;
	}

    /**
     * @brief Get the note on velocity for a given note
     *
     * @param channel (1-based)
     * @param noteNumber
     * @return uint8_t
     */
	uint8_t getNoteVelocity(int channel, int noteNumber) const noexcept
	{
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(noteNumber >= 0 && noteNumber <= 127);
        channel = translateSfzChannelToMidi(channel);
        return lastNoteVelocities[channel][noteNumber];
	}

    void pitchBendEvent(int channel, int pitchBendValue) noexcept
    {
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(pitchBendValue >= -8192 && pitchBendValue <= 8192);
        channel = translateSfzChannelToMidi(channel);
        pitchBends[channel] = pitchBendValue;
    }

    int getPitchBend(int channel) const noexcept
    {
        ASSERT(channel >= 1 && channel <= 16);
        channel = translateSfzChannelToMidi(channel);
        return pitchBends[channel];
    }

    void ccEvent(int channel, int ccNumber, uint8_t ccValue) noexcept
    {
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(ccNumber >= 0 && ccNumber <= 127);
        ASSERT(ccValue >= 0 && ccValue <= 127);
        channel = translateSfzChannelToMidi(channel);
        cc[channel][ccNumber] = ccValue;
    }

    uint8_t getCCValue(int channel, int ccNumber) const noexcept
    {
        ASSERT(channel >= 1 && channel <= 16);
        ASSERT(ccNumber >= 0 && ccNumber <= 127);
        channel = translateSfzChannelToMidi(channel);
        return cc[channel][ccNumber];
    }

    const CCValueArray& getCCArray(int channel) const noexcept
    {
        ASSERT(channel >= 1 && channel <= 16);
        channel = translateSfzChannelToMidi(channel);
        return cc[channel];
    }

    void reset() noexcept
    {
        for (auto& channelArray: lastNoteVelocities)
            for (auto& velocity: channelArray)
                velocity = 0;

        for (auto& channelArray: cc)
            for (auto& ccValue: channelArray)
                ccValue = 0;

        for (auto& bendValue: pitchBends)
            bendValue = 0;
    }

private:
    template<class T>
    using ChannelArray = std::array<T, 16>;
    template<class T>
    using MidiArray = std::array<T, 128>;
    using NoteOnTime = std::chrono::steady_clock::time_point;
    /**
     * @brief Stores the note on times.
     *
     */
	ChannelArray<MidiArray<NoteOnTime>> noteOnTimes { };
    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
	ChannelArray<MidiArray<uint8_t>> lastNoteVelocities { };
    /**
     * @brief Current known values for the CCs.
     *
     */
	ChannelArray<CCValueArray> cc;
    /**
     * Pitch bend status
     */
    ChannelArray<int> pitchBends { 0 };
};
}
