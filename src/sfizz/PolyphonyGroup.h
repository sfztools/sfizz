#pragma once
#include "Region.h"
#include "Voice.h"
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
    void removeVoice(Voice* voice)
    {
        auto it = absl::c_find(voices, voice);
        if (it != voices.end()) {
            std::iter_swap(it, voices.rbegin().base());
            voices.pop_back();
        }
    }
    const std::vector<Voice*>& getActiveVoices() const { return voices; }
private:
    unsigned polyphonyLimit { config::maxVoices };
    std::vector<Voice*> voices;
};

}
