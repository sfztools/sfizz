#pragma once
#include "SfzFilter.h"
#include "EQDescription.h"
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
     * @brief Setup a new EQ based on an EQ description.
     *
     * @param description the EQ description
     * @param numChannels the number of channels for the EQ
     * @param description the triggering velocity/value
     */
    void setup(const EQDescription& description, unsigned numChannels, float velocity);
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
