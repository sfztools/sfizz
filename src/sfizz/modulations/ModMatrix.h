// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../NumericId.h"
#include <memory>
#include <cstdint>

namespace sfz {

class ModKey;
class ModGenerator;
class Voice;

/**
 * @brief Modulation matrix
 */
class ModMatrix {
public:
    ModMatrix();
    ~ModMatrix();

    struct SourceIdTag;
    struct TargetIdTag;

    //! Identifier of a modulation source
    typedef NumericId<SourceIdTag> SourceId;

    //! Identifier of a modulation target
    typedef NumericId<TargetIdTag> TargetId;

    /**
     * @brief Reset the matrix to the empty state.
     */
    void clear();

    /**
     * @brief Change the sample rate.
     *
     * @param sampleRate new sample rate
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Resize the modulation buffers.
     *
     * @param samplesPerBlock new block size
     */
    void setSamplesPerBlock(unsigned samplesPerBlock);

    /**
     * @brief Register a modulation source inside the matrix.
     * If it is already present, it just returns the existing id.
     *
     * @param key source key
     * @param gen generator
     * @param flags source flags
     */
    SourceId registerSource(const ModKey& key, ModGenerator& gen);

    /**
     * @brief Register a modulation target inside the matrix.
     *
     * @param key target key
     * @param region target region
     * @param flags target flags
     */
    TargetId registerTarget(const ModKey& key);

    /**
     * @brief Look up a source by key.
     *
     * @param key source key
     */
    SourceId findSource(const ModKey& key);

    /**
     * @brief Look up a target by key.
     *
     * @param key target key
     */
    TargetId findTarget(const ModKey& key);

    /**
     * @brief Connect a source and a destination inside the matrix.
     *
     * @param sourceId source of the connection
     * @param targetId target of the connection
     * @return true if the connection was successfully made, otherwise false
     */
    bool connect(SourceId sourceId, TargetId targetId);

    /**
     * @brief Reinitialize modulation sources overall.
     * This must be called once after setting up the matrix.
     */
    void init();

    /**
     * @brief Reinitialize modulation source for a given voice.
     * This must be called first after a voice enters active state.
     */
    void initVoice(NumericId<Voice> voiceId);

    /**
     * @brief Start modulation processing for the entire cycle.
     * This clears all the buffers.
     *
     * @param numFrames
     */
    void beginCycle(unsigned numFrames);

    /**
     * @brief End modulation processing for the entire cycle.
     * This performs a dummy run of any unused modulations.
     */
    void endCycle();

    /**
     * @brief Start modulation processing for a given voice.
     * This clears all the buffers which are per-voice.
     *
     * @param voiceId the identifier of the current voice
     */
    void beginVoice(NumericId<Voice> voiceId);

    /**
     * @brief End modulation processing for a given voice.
     * This performs a dummy run of any unused modulations which are per-cycle.
     */
    void endVoice();

    /**
     * @brief Get the modulation buffer for the given target.
     * If the target does not exist, the result is null.
     *
     * @param targetId identifier of the modulation target
     */
    float* getModulation(TargetId targetId);

    /**
     * @brief Get the modulation buffer for the given target.
     * Same as `getModulation`, but accepting a key directly.
     *
     * @param targetKey key of the modulation target
     */
    float* getModulationByKey(const ModKey& targetKey)
        { return getModulation(findTarget(targetKey)); }

    /**
     * @brief Return whether the target identifier is valid.
     *
     * @param id
     */
    bool validTarget(TargetId id) const;

    /**
     * @brief Return whether the source identifier is valid.
     *
     * @param id
     */
    bool validSource(SourceId id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sfz