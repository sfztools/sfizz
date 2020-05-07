// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Buffer.h"
#include "absl/container/flat_hash_map.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace sfz { class ModKey; }

namespace std {
    template <> struct hash<sfz::ModKey> {
        size_t operator()(const sfz::ModKey &key) const;
    };
}

namespace sfz {

/**
 * @brief Identifier of a modulation source or target
 */
class ModKey {
public:
    //! Identifier, a letter-only hash
    uint64_t id = 0;
    //! List of values which identify the key uniquely, along with the hash
    std::vector<float> params;

    // eg. `N` in `lfoN`, `N, X` in `lfoN_eqX`
    //     `nsteps, curve, smooth` for controllers

public:
    bool operator==(const ModKey &other) const noexcept;
    bool operator!=(const ModKey &other) const noexcept;
};


/**
 * @brief Modulation bit flags (S=source, T=target, ST=either)
 */
enum ModFlags {
    //! This modulation is global (the default). (ST)
    kModIsPerCycle = 0,
    //! This modulation is updated separately for every voice (ST)
    kModIsPerVoice = 1 << 1,
    //! This target is additive (the default). (T)
    kModIsAdditive = 0,
    //! This target is multiplicative (T)
    kModIsMultiplicative = 1 << 2,
};

/**
 * @brief Generator for modulation sources
 */
class ModGenerator {
public:
    virtual ~ModGenerator() {}

    /**
     * @brief Generate a cycle of the modulator
     *
     * @param sourceKey source key
     * @param voiceNum voice number if the generator is per-voice, otherwise undefined
     * @param buffer output buffer
     * @param numFrames number of frames to generate
     */
    virtual void generateModulation(ModKey sourceKey, int32_t voiceNum, float *buffer, unsigned numFrames) = 0;
};

/**
 * @brief Modulation matrix
 */
class ModMatrix {
public:
    ModMatrix();
    ~ModMatrix();

    //! Identifier of a modulation source
    struct SourceId { int32_t index = -1; };

    //! Identifier of a modulation target
    struct TargetId { int32_t index = -1; };

    /**
     * @brief Reset the matrix to the empty state.
     */
    void clear();

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
     * @param genOwned whether the source takes ownership of the generator
     * @param flags source flags
     */
    SourceId registerSource(ModKey key, ModGenerator* gen, bool genOwned, int32_t flags);

    /**
     * @brief Register a modulation target inside the matrix.
     *
     * @param key target key
     * @param region target region
     * @param flags target flags
     */
    TargetId registerTarget(ModKey key, uint32_t region, int32_t flags);

    /**
     * @brief Look up a source by key.
     *
     * @param key source key
     */
    SourceId findSource(ModKey key);

    /**
     * @brief Look up a target by key.
     *
     * @param key target key
     * @param index of target region
     */
    TargetId findTarget(ModKey key, uint32_t region);

    /**
     * @brief Connect a source and a destination inside the matrix.
     *
     * @param sourceId source of the connection
     * @param targetId target of the connection
     * @param depth factor by which the source signal affects the target
     * @return true if the connection was successfully made, otherwise false
     */
    bool connect(SourceId sourceId, TargetId targetId, float depth);

    /**
     * @brief Start modulation processing for the entire cycle.
     * This clears all the buffers.
     *
     * @param numFrames
     */
    void beginCycle(unsigned numFrames);

    /**
     * @brief Start modulation processing for a given voice.
     * This clears all the buffers which are per-voice.
     *
     * @param voiceNum the number of the current voice
     */
    void beginVoice(unsigned voiceNum);

    /**
     * @brief Get the modulation buffer for the given target.
     * If the target does not exist, the result is null.
     *
     * @param targetId identifier of the modulation target
     */
    float* getModulation(TargetId targetId);

    /**
     * @brief Return whether the target identifier is valid.
     *
     * @param id
     */
    bool validTarget(TargetId id) const
    {
        return static_cast<uint32_t>(id.index) < _targets.size();
    }

    /**
     * @brief Return whether the source identifier is valid.
     *
     * @param id
     */
    bool validSource(SourceId id) const
    {
        return static_cast<uint32_t>(id.index) < _sources.size();
    }

private:
    uint32_t _samplesPerBlock {};

    uint32_t _numFrames {};
    uint32_t _voiceNum {};

    struct Source {
        ModKey key;
        ModGenerator* gen {};
        std::unique_ptr<ModGenerator> genOwnerPtr;
        int32_t flags {};
        bool bufferReady {};
        Buffer<float> buffer;
    };

    struct ConnectionData {
        float depthToTarget {};
    };

    struct Target {
        ModKey key;
        uint32_t region {};
        int32_t flags {};
        absl::flat_hash_map<uint32_t, ConnectionData> connectedSources;
        bool bufferReady {};
        Buffer<float> buffer;
    };

    typedef std::pair<ModKey, uint32_t> TargetKey;

    absl::flat_hash_map<ModKey, uint32_t> _sourceIndex;
    absl::flat_hash_map<TargetKey, uint32_t> _targetIndex;

    std::vector<Source> _sources;
    std::vector<Target> _targets;

    Buffer<float> _temp;
};

} // namespace sfz
