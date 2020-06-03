#pragma once
#include "Region.h"
#include "Voice.h"
#include <vector>

namespace sfz
{

class RegionSet {
public:
    void setPolyphonyLimit(unsigned limit)
    {
        polyphonyLimit = limit;
        voices.reserve(limit);
    }
    unsigned getPolyphonyLimit() const { return polyphonyLimit; }
    void addRegion(Region* region)
    {
        if (absl::c_find(regions, region) == regions.end())
            regions.push_back(region);
    }
    void addSubset(RegionSet* group)
    {
        if (absl::c_find(subsets, group) == subsets.end())
            subsets.push_back(group);
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
    RegionSet* getParent() const { return parent; }
    void setParent(RegionSet* parent) { this->parent = parent; }
    const std::vector<Voice*>& getActiveVoices() const { return voices; }
    const std::vector<Region*>& getRegions() const { return regions; }
    const std::vector<RegionSet*>& getSubsets() const { return subsets; }
private:
    RegionSet* parent { nullptr };
    std::vector<Region*> regions;
    std::vector<RegionSet*> subsets;
    std::vector<Voice*> voices;
    unsigned polyphonyLimit { config::maxVoices };
};

}
