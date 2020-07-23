#pragma once
#include "SfzFilter.h"
#include "FilterDescription.h"
#include "MidiState.h"
#include "Defaults.h"
#include <vector>
#include <memory>
#include <mutex>

namespace sfz
{

class FilterHolder
{
public:
    FilterHolder() = delete;
    FilterHolder(const MidiState& state);
    /**
     * @brief Setup a new filter based on a filter description, and a triggering note parameters.
     *
     * @param description the filter description
     * @param numChannels the number of channels
     * @param noteNumber the triggering note number
     * @param velocity the triggering note velocity/value
     */
    void setup(const FilterDescription& description, unsigned numChannels, int noteNumber = static_cast<int>(Default::filterKeycenter), float velocity = 0);
    /**
     * @brief Process a block of stereo inputs
     *
     * @param inputs
     * @param outputs
     * @param numFrames
     */
    void process(const float** inputs, float** outputs, unsigned numFrames);
    /**
     * @brief Returns the last value of the cutoff for the filter
     *
     * @return float
     */
    float getLastCutoff() const;
    /**
     * @brief Returns the last value of the resonance for the filter
     *
     * @return float
     */
    float getLastResonance() const;
    /**
     * @brief Returns the last value of the gain for the filter
     *
     * @return float
     */
    float getLastGain() const;
    /**
     * @brief Set the sample rate for a filter
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate);
private:
    /**
     * Reset the filter. Is called internally when using setup().
     */
    void reset();
    const MidiState& midiState;
    const FilterDescription* description;
    Filter filter;
    float baseCutoff { Default::filterCutoff };
    float baseResonance { Default::filterResonance };
    float baseGain { Default::filterGain };
    float lastCutoff { Default::filterCutoff };
    float lastResonance { Default::filterResonance };
    float lastGain { Default::filterGain };
    using filterRandomDist = std::uniform_int_distribution<int>;
    filterRandomDist dist { 0, sfz::Default::filterRandom };
};

using FilterHolderPtr = std::shared_ptr<FilterHolder>;

class FilterPool
{
public:
    FilterPool() = delete;
    /**
     * @brief Construct a new Filter Pool object
     *
     * @param state the associated midi state
     * @param numFilters the number of inactive filters to hold in the pool
     */
    FilterPool(const MidiState& state, int numFilters = config::filtersInPool);
    /**
     * @brief Get a filter object to use in Voices
     *
     * @param description the filter description to bind to the filter
     * @param numChannels the number of channels in the underlying filter
     * @param noteNumber the triggering note number
     * @param velocity the triggering note velocity
     * @return FilterHolderPtr release this when done with the filter; no deallocation will be done
     */
    FilterHolderPtr getFilter(const FilterDescription& description, unsigned numChannels, int noteNumber = static_cast<int>(Default::filterKeycenter), float velocity = 0);
    /**
     * @brief Get the number of active filters
     *
     * @return size_t
     */
    size_t getActiveFilters() const;
    /**
     * @brief Set the number of filters in the pool. This function may sleep and should be called from a background thread.
     * No filters will be distributed during the reallocation of filters. Existing running filters are kept. If the target
     * number of filters is less that the number of active filters, the function will not remove them and you may need
     * to call it again after existing filters have run out.
     *
     * @param numFilters
     * @return size_t the actual number of filters in the pool
     */
    size_t setNumFilters(size_t numFilters);
    /**
     * @brief Set the sample rate for all filters
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate);
private:
    std::mutex filterGuard;
    float sampleRate { config::defaultSampleRate };
    const MidiState& midiState;
    std::vector<FilterHolderPtr> filters;
};
}
