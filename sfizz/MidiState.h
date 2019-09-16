#include <chrono>
#include <array>

namespace sfz
{
	inline std::array<std::chrono::steady_clock::time_point, 128> noteOnTimes { };
	inline void setNoteOnTime(int noteNumber)
	{
		if (noteNumber >= 0 && noteNumber < 128)
			noteOnTimes[noteNumber] = std::chrono::steady_clock::now();
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
}