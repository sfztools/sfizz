// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Smoothers.h"

namespace sfz {

Smoother::Smoother()
{
}

void Smoother::setSmoothing(uint8_t smoothValue, float sampleRate)
{
    smoothing = (smoothValue > 0);
    if (smoothing) {
        filter.setGain(std::tan(1.0f / (2 * Default::smoothTauPerStep * smoothValue * sampleRate)));
    }
}

void Smoother::reset(float value)
{
    filter.reset(value);
}

void Smoother::process(absl::Span<const float> input, absl::Span<float> output)
{
    if (smoothing)
        filter.processLowpass(input, output);
    else
        copy<float>(input, output);
}

}
