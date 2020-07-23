#pragma once
#include "SfzFilter.h"
#include "EQDescription.h"
#include "MidiState.h"
#include <vector>
#include <memory>
#include <mutex>

namespace sfz
{

class EQHolder
{
public:
    EQHolder() = delete;
    EQHolder(const MidiState& state);
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
     * @brief Returns the last value of the frequency for the EQ
     *
     * @return float
     */
    float getLastFrequency() const;
    /**
     * @brief Returns the last value of the bandwitdh for the EQ
     *
     * @return float
     */
    float getLastBandwidth() const;
    /**
     * @brief Returns the last value of the gain for the EQ
     *
     * @return float
     */
    float getLastGain() const;
    /**
     * @brief Set the sample rate for the EQ
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
    const EQDescription* description;
    FilterEq eq;
    float baseBandwidth { Default::eqBandwidth };
    float baseFrequency { Default::eqFrequency1 };
    float baseGain { Default::eqGain };
    float lastBandwidth { Default::eqBandwidth };
    float lastFrequency { Default::eqFrequency1 };
    float lastGain { Default::eqGain };
};

using EQHolderPtr = std::shared_ptr<EQHolder>;

class EQPool
{
public:
    EQPool() = delete;
    /**
     * @brief Construct a new EQPool object
     *
     * @param state the associated midi state
     * @param numEQs the number of inactive EQs to hold in the pool
     */
    EQPool(const MidiState& state, int numEQs = config::filtersInPool);
    /**
     * @brief Get an EQ object to use in Voices
     *
     * @param description the filter description to bind to the EQ
     * @param numChannels the number of channels for the EQ
     * @param velocity the triggering note velocity/value
     * @return EQHolderPtr release this when done with the filter; no deallocation will be done
     */
    EQHolderPtr getEQ(const EQDescription& description, unsigned numChannels, float velocity);
    /**
     * @brief Get the number of active EQs
     *
     * @return size_t
     */
    size_t getActiveEQs() const;
    /**
     * @brief Set the number of EQs in the pool. This function may sleep and should be called from a background thread.
     * No EQs will be distributed during the reallocation of EQs. Existing running EQs are kept. If the target
     * number of EQs is less that the number of active EQs, the function will not remove them and you may need
     * to call it again after existing EQs have run out.
     *
     * @param numEQs
     * @return size_t the actual number of EQs in the pool
     */
    size_t setnumEQs(size_t numEQs);
    /**
     * @brief Set the sample rate for all EQs
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate);
private:
    std::mutex eqGuard;
    float sampleRate { config::defaultSampleRate };
    const MidiState& midiState;
    std::vector<EQHolderPtr> eqs;
};
}
