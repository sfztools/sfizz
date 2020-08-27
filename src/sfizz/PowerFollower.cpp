// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PowerFollower.h"
#include "Defaults.h"
#include "SIMDHelpers.h"
#include <absl/types/span.h>

namespace sfz {

PowerFollower::PowerFollower()
    : sampleRate_(config::defaultSampleRate),
      samplesPerBlock_(config::defaultSamplesPerBlock),
      tempBuffer_(new float[config::defaultSamplesPerBlock])
{
    updateTrackingFactor();
}

void PowerFollower::setSampleRate(float sampleRate) noexcept
{
    if (sampleRate_ != sampleRate) {
        sampleRate_ = sampleRate;
        updateTrackingFactor();
    }
}

void PowerFollower::setSamplesPerBlock(unsigned samplesPerBlock)
{
    if (samplesPerBlock_ != samplesPerBlock) {
        tempBuffer_.reset(new float[samplesPerBlock]);
        samplesPerBlock_ = samplesPerBlock;
        updateTrackingFactor();
    }
}

void PowerFollower::process(AudioSpan<float> buffer) noexcept
{
    size_t numFrames = buffer.getNumFrames();
    if (numFrames == 0)
        return;

    absl::Span<float> tempBuffer(tempBuffer_.get(), numFrames);

    copy(buffer.getConstSpan(0), tempBuffer);
    for (unsigned i = 1; i < buffer.getNumChannels(); ++i)
        add(buffer.getConstSpan(i), tempBuffer);

    const float meanPower = meanSquared<float>(tempBuffer);

    const float attackFactor = static_cast<float>(buffer.getNumFrames()) * attackTrackingFactor_;
    const float releaseFactor = static_cast<float>(buffer.getNumFrames()) * releaseTrackingFactor_;

    meanChannelPower_ = max(
        meanChannelPower_ * (1 - attackFactor) + meanPower * attackFactor,
        meanChannelPower_ * (1 - releaseFactor) + meanPower * releaseFactor
    );
}

void PowerFollower::clear() noexcept
{
    meanChannelPower_ = 0;
}

void PowerFollower::updateTrackingFactor() noexcept
{
    // Protect the envelope follower against blowups
    const auto maxTrackingFactor = sampleRate_ / samplesPerBlock_;
    attackTrackingFactor_ =  min(config::powerFollowerAttackFactor, maxTrackingFactor) / sampleRate_;
    releaseTrackingFactor_ =  min(config::powerFollowerReleaseFactor, maxTrackingFactor) / sampleRate_;
}

} // namespace sfz
