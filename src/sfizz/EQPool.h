#pragma once
#include "SfzFilter.h"
#include "Region.h"
#include "Resources.h"
#include "utility/SpinMutex.h"
#include <vector>
#include <memory>
#include <mutex>

namespace sfz
{

class EQHolder
{
public:
    EQHolder() = delete;
    EQHolder(const Resources& resources);
    /**
     * @brief Setup a new EQ from a region and an index
     *
     * @param description   the region from which we take the EQ
     * @param eqId          the EQ index in the region
     * @param description   the triggering velocity/value
     */
    void setup(const Region& region, unsigned eqId, float velocity);
    /**
     * @brief Process a block of stereo inputs
     *
     * @param inputs
     * @param outputs
     * @param numFrames
     */
    void process(const float** inputs, float** outputs, unsigned numFrames);
    /**
     * @brief Set the sample rate for the EQ
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
    const EQDescription* description;
    std::unique_ptr<FilterEq> eq;
    float baseBandwidth { Default::eqBandwidth };
    float baseFrequency { Default::eqFrequency1 };
    float baseGain { Default::eqGain };
    ModMatrix::TargetId eqGainTarget;
    ModMatrix::TargetId eqFrequencyTarget;
    ModMatrix::TargetId eqBandwidthTarget;
};

}
