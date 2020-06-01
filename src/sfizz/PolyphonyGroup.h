#pragma once
#include "Region.h"
#include "Voice.h"
#include "absl/algorithm/container.h"

namespace sfz
{
class PolyphonyGroup {
public:
    PolyphonyGroup() = delete;
    PolyphonyGroup(unsigned id)
    : id(id) {}
    void setPolyphonyLimit(unsigned limit)
    {
        polyphonyLimit = limit;
        voices.reserve(limit);
    }
    unsigned getPolyphonyLimit() { return polyphonyLimit; }
    unsigned getID() const { return id; }
    void addRegion(Region* region)
    {
        if (absl::c_find(regions, region) == regions.end())
            regions.push_back(region);
    }
    void addVoice(Voice* voice)
    {
        if (absl::c_find(voices, voice) == voices.end())
            voices.push_back(voice);
    }
private:
    unsigned id;
    unsigned polyphonyLimit;
    std::vector<Voice*> voices;
    std::vector<Region*> regions;
};

}
