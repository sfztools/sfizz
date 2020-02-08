#pragma once
#include "SfzFilter.h"
#include "FilterDescription.h"
#include "MidiState.h"
#include <vector>
#include <memory>

namespace sfz
{

class FilterHolder
{
public:
    FilterHolder() = delete;
    FilterHolder(const MidiState& state);
    void reset();
    void setup(const FilterDescription& description, int noteNumber = static_cast<int>(Default::filterKeycenter), uint8_t velocity = 0);
    void process(const float** inputs, float** outputs, unsigned numFrames);
private:
    const MidiState& midiState;
    const FilterDescription* description;
    Filter filter;
    float baseCutoff { Default::filterCutoff };
    float baseResonance { Default::filterResonance };
    float baseGain { Default::filterGain };
    using filterRandomDist = std::uniform_int_distribution<int>;
    filterRandomDist dist { 0, sfz::Default::filterRandom };
};

using FilterHolderPtr = std::shared_ptr<FilterHolder>;

class FilterPool
{
public:
    FilterPool() = delete;
    FilterPool(const MidiState& state, int numFilters = config::defaultNumFilters);
    FilterHolderPtr getFilter(const FilterDescription& description, int noteNumber = static_cast<int>(Default::filterKeycenter), uint8_t velocity = 0);
    int getActiveFilters() const;
    void setNumFilters(int numFilters);
private:
    const MidiState& midiState;
    std::vector<FilterHolderPtr> filters;
};
}
