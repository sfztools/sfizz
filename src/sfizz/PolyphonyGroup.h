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
    void addVoice(Voice* voice)
    {
        if (absl::c_find(voices, voice) == voices.end())
            voices.push_back(voice);
    }
    const std::vector<Voice*>& getActiveVoices() const { return voices; }
private:
    unsigned polyphonyLimit;
    std::vector<Voice*> voices;
};

}
