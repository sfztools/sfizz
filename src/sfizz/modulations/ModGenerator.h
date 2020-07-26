// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../NumericId.h"
#include <absl/types/span.h>
#include <cstdint>

namespace sfz {

class ModKey;
class Voice;

/**
 * @brief Generator for modulation sources
 */
class ModGenerator {
public:
    virtual ~ModGenerator() {}

    /**
     * @brief Set the sample rate
     */
    virtual void setSampleRate(double sampleRate) = 0;

    /**
     * @brief Set the maximum block size
     */
    virtual void setSamplesPerBlock(unsigned count) = 0;

    /**
     * @brief Initialize the generator.
     *
     * @param sourceKey identifier of the source to initialize
     * @param voiceId the particular voice to initialize, if per-voice
     */
    virtual void init(const ModKey& sourceKey, NumericId<Voice> voiceId) = 0;

    /**
     * @brief Generate a cycle of the modulator
     *
     * @param sourceKey source key
     * @param voiceNum voice number if the generator is per-voice, otherwise undefined
     * @param buffer output buffer
     */
    virtual void generate(const ModKey& sourceKey, NumericId<Voice> voiceNum, absl::Span<float> buffer) = 0;
};

} // namespace sfz
