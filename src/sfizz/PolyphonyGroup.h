#pragma once
#include "Region.h"
#include "Voice.h"
#include "SwapAndPop.h"
#include "absl/algorithm/container.h"

namespace sfz
{
class PolyphonyGroup {
public:
    void setPolyphonyLimit(unsigned limit)
    {
        polyphonyLimit = limit;
        voices.reserve(limit);
    }
    unsigned getPolyphonyLimit() const { return polyphonyLimit; }
    void registerVoice(Voice* voice)
    {
        if (absl::c_find(voices, voice) == voices.end())
            voices.push_back(voice);
    }
    void removeVoice(const Voice* voice)
    {
        swapAndPopFirst(voices, [voice](const Voice* v) { return v == voice; });
    }
    const std::vector<Voice*>& getActiveVoices() const { return voices; }
    std::vector<Voice*>& getActiveVoices() { return voices; }
private:
    unsigned polyphonyLimit { config::maxVoices };
    std::vector<Voice*> voices;
};

}
