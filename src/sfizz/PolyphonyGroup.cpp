#include "PolyphonyGroup.h"

void sfz::PolyphonyGroup::setPolyphonyLimit(unsigned limit) noexcept
{
    polyphonyLimit = limit;
    voices.reserve(limit);
}

void sfz::PolyphonyGroup::registerVoice(Voice* voice) noexcept
{
    if (absl::c_find(voices, voice) == voices.end())
        voices.push_back(voice);
}

void sfz::PolyphonyGroup::removeVoice(const Voice* voice) noexcept
{
    swapAndPopFirst(voices, [voice](const Voice* v) { return v == voice; });
}

unsigned sfz::PolyphonyGroup::numPlayingVoices() const noexcept
{
    return absl::c_count_if(voices, [](const Voice* v) {
        return !v->releasedOrFree();
    });
}
