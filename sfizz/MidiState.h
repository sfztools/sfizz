#pragma once
#include <chrono>
#include <array>
#include "SfzHelpers.h"
#include "compat/utils.h"

namespace sfz
{
struct MidiState
{
	inline void noteOn(int noteNumber, uint8_t velocity)
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			lastNoteVelocities[noteNumber] = velocity;
			noteOnTimes[noteNumber] = std::chrono::steady_clock::now();
		}
	}

	inline float getNoteDuration(int noteNumber) const
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			const auto noteOffTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[noteNumber]);
			return duration.count();
		}

		return 0.0f;
	}

	inline uint8_t getNoteVelocity(int noteNumber) const
	{
		if (noteNumber >= 0 && noteNumber < 128)
			return lastNoteVelocities[noteNumber];

		return 0;
	}
	std::array<std::chrono::steady_clock::time_point, 128> noteOnTimes { };
	std::array<uint8_t, 128> lastNoteVelocities { };
	CCValueArray cc;
};
}
