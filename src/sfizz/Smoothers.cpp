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

bool Smoother::process(absl::Span<const float> input, absl::Span<float> output, bool canShortcut)
{
    CHECK_SPAN_SIZES(input, output);
    if (input.size() == 0)
        return false;

    if (canShortcut) {
        float in = input.front();
        float rel = std::abs(in - current()) / (std::abs(in) + config::virtuallyZero);
        canShortcut = rel < config::smoothingShortcutThreshold;
    }

    bool didShortcut = false;

    if (canShortcut) {
        if (input.data() != output.data())
            copy<float>(input, output);

        filter.reset(input.back());
        didShortcut = true;
    } else if (smoothing) {
        filter.processLowpass(input, output);
    } else if (input.data() != output.data()) {
        copy<float>(input, output);
        didShortcut = true;
    }

    return didShortcut;
}

}
