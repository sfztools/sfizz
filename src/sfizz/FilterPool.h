#pragma once
#include "SfzFilter.h"
#include "FilterDescription.h"
#include "Resources.h"
#include "Defaults.h"
#include "utility/SpinMutex.h"
#include <vector>
#include <memory>
#include <mutex>

namespace sfz
{

class FilterHolder
{
public:
    FilterHolder() = delete;
    FilterHolder(const Resources& resources);
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
     * @brief Set the sample rate for a filter
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate);
    /**
     * Reset the filter.
     */
    void reset();
private:
    const Resources& resources;
    const FilterDescription* description;
    std::unique_ptr<Filter> filter;
    float baseCutoff { Default::filterCutoff };
    float baseResonance { Default::filterResonance };
    float baseGain { Default::filterGain };
    ModMatrix::TargetId filterGainTarget;
    ModMatrix::TargetId filterCutoffTarget;
    ModMatrix::TargetId filterResonanceTarget;
    using filterRandomDist = std::uniform_int_distribution<int>;
    filterRandomDist dist { 0, sfz::Default::filterRandom };
};

}
