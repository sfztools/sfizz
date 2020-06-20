// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Config.h"
#include "Defaults.h"
#include "MathHelpers.h"
#include "SfzHelpers.h"
#include "OnePoleFilter.h"
#include "SIMDHelpers.h"
#include <array>

namespace sfz {
/**
 * @brief Wrapper class for a one pole filter smoother
 *
 */
class Smoother {
public:
    Smoother();
    /**
     * @brief Set the filter cutoff based on the sfz smoothing value
     * and the sample rate.
     *
     * @param smoothValue
     * @param sampleRate
     */
    void setSmoothing(uint8_t smoothValue, float sampleRate);
    /**
     * @brief Reset the filter state to a given value
     *
     * @param value
     */
    void reset(float value = 0.0f);
    /**
     * @brief Process a span of data. Input and output can refer to the same
     * memory.
     *
     * @param input
     * @param output
     */
    void process(absl::Span<const float> input, absl::Span<float> output);

private:
    bool smoothing { false };
    OnePoleFilter<float> filter {};
};

}
