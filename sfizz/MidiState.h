#include <chrono>
#include <array>

namespace sfz
{
	inline std::array<std::chrono::steady_clock::time_point, 128> noteOnTimes { };
	inline std::array<uint8_t, 128> lastNoteVelocities { };
	inline void noteOn(int noteNumber, uint8_t velocity)
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			lastNoteVelocities[noteNumber] = velocity;
			noteOnTimes[noteNumber] = std::chrono::steady_clock::now();
		}
	}

	inline float getNoteDuration(int noteNumber)
	{
		if (noteNumber >= 0 && noteNumber < 128) {
			const auto noteOffTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[noteNumber]);
			return duration.count();
		}

		return 0.0f;
	}

	inline uint8_t getNoteVelocity(int noteNumber)
	{
		if (noteNumber >= 0 && noteNumber < 128)
			return lastNoteVelocities[noteNumber];

		return 0;
	}
}