#pragma once
#include "Region.h"
#include "Voice.h"
#include <vector>

namespace sfz
{

class Group {
public:
    void setPolyphonyLimit(unsigned limit);
    unsigned getPolyphonyLimit() { return polyphonyLimit; }
    void addRegion(Region* region)
    {
        if (absl::c_find(regions, region) == regions.end())
            regions.push_back(region);
    }
    void addSubgroup(Group* group)
    {
        if (absl::c_find(subgroups, group) == subgroups.end())
            subgroups.push_back(group);
    }
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
    const std::vector<Voice*>& getActiveVoices() { return voices; }
    const std::vector<Region*>& getRegions() { return regions; }
    const std::vector<Group*>& getSubgroups() { return subgroups; }
private:
    Group* parent { nullptr };
    std::vector<Region*> regions;
    std::vector<Group*> subgroups;
    std::vector<Voice*> voices;
    unsigned polyphonyLimit;
};

}
