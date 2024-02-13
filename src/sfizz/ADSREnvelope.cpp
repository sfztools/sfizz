// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ADSREnvelope.h"
#include "Config.h"
#include "SIMDHelpers.h"
#include "MathHelpers.h"

namespace sfz {

using Float = ADSREnvelope::Float;

int ADSREnvelope::secondsToSamples(Float timeInSeconds) const noexcept
{
    if (timeInSeconds <= 0)
        return Float(0);

    return static_cast<int>(timeInSeconds * sampleRate);
};

Float ADSREnvelope::secondsToLinRate(Float timeInSeconds) const noexcept
{
    if (timeInSeconds <= 0)
        return Float(1);

    return 1 / (sampleRate * timeInSeconds);
};

Float ADSREnvelope::secondsToExpRate(Float timeInSeconds) const noexcept
{
    if (timeInSeconds <= 0)
        return Float(1);

    timeInSeconds = std::max(Float(Default::offTime), timeInSeconds);
    return 1.0 / (sampleRate * timeInSeconds);
};

void ADSREnvelope::reset(const EGDescription& desc, const Region& region, int delay, float velocity, float sampleRate) noexcept
{
    this->sampleRate = sampleRate;
    desc_ = &desc;
    dynamic_ = desc.dynamic;
    triggerVelocity_ = velocity;
    currentState = State::Delay; // Has to be before the update
    updateValues(delay);
    this->attackShape = desc.attack_shape;
    this->decayShape = desc.decay_shape;
    this->releaseShape = desc.release_shape;
    releaseDelay = 0;
    shouldRelease = false;
    freeRunning = (
        (this->sustain <= Float(config::sustainFreeRunningThreshold))
        || (region.loopMode == LoopMode::one_shot && region.isOscillator())
    );
    currentValue = this->start;
    releaseValue = 0;
}

void ADSREnvelope::updateValues(int delay) noexcept
{
    if (currentState == State::Delay)
        this->delay = delay + secondsToSamples(desc_->getDelay(midiState_, curveSet_, triggerVelocity_, delay));
    this->attackStep = secondsToLinRate(desc_->getAttack(midiState_, curveSet_, triggerVelocity_, delay));
    this->decayRate = secondsToExpRate(desc_->getDecay(midiState_, curveSet_, triggerVelocity_, delay));
    this->releaseRate = secondsToExpRate(desc_->getRelease(midiState_, curveSet_, triggerVelocity_, delay));
    this->hold = secondsToSamples(desc_->getHold(midiState_, curveSet_, triggerVelocity_, delay));
    this->sustain = clamp(desc_->getSustain(midiState_, curveSet_, triggerVelocity_, delay), 0.0f, 1.0f);
    this->start = clamp(desc_->getStart(midiState_, curveSet_, triggerVelocity_, delay), 0.0f, 1.0f);
    sustainThreshold = this->sustain + config::virtuallyZero;
}

void ADSREnvelope::getBlock(absl::Span<Float> output) noexcept
{
    if (dynamic_) {
        int processed = 0;
        int remaining = static_cast<int>(output.size());
        while(remaining > 0) {
            updateValues(processed);
            int chunkSize = min(config::processChunkSize, remaining);
            getBlockInternal(output.subspan(processed, chunkSize));
            processed += chunkSize;
            remaining -= chunkSize;
        }
    } else {
        getBlockInternal(output);
    }
}

void ADSREnvelope::getBlockInternal(absl::Span<Float> output) noexcept
{
    State currentState = this->currentState;
    Float currentValue = this->currentValue;
    bool shouldRelease = this->shouldRelease;
    int releaseDelay = this->releaseDelay;
    Float transitionDelta = this->transitionDelta;
    Float attackCount = this->attackCount;
    Float decayCount = this->decayCount;
    Float releaseCount = this->releaseCount;
    Float releaseValue = this->releaseValue;

    while (!output.empty()) {
        size_t count = 0;
        size_t size = output.size();

        if (shouldRelease) {
            if (releaseDelay > 0) {
                // prevent computing the segment further than release point
                size = std::min<size_t>(size, releaseDelay);
            } else if (releaseDelay == 0 && delay < 0) {
                // release takes effect this frame
                currentState = State::Release;
                releaseDelay = -1;
                releaseValue = currentValue;
                releaseCount = 1;
            }
        }

        Float previousValue;

        switch (currentState) {
        case State::Delay:
            attackCount = 0;
            while (count < size && delay-- > 0) {
                currentValue = start;
                output[count++] = currentValue;
            }
            if (delay <= 0)
            {
                currentState = State::Attack;
            }
            break;
        case State::Attack:
            while (count < size && (currentValue) < 1)
            {
                if (attackShape < 0)
                    currentValue = start + (1 - start) * pow(attackCount, -attackShape + 1.0f);
                else
                    currentValue = start + (1 - start) * pow(attackCount, 1.0f / (attackShape + 1.0f));
                output[count++] = currentValue;
                attackCount = min(attackCount + attackStep, 1.0f);
            }
            if (currentValue >= 1) {
                currentValue = 1;
                currentState = State::Hold;
            }
            break;
        case State::Hold:
            decayCount = 1;
            while (count < size && hold-- > 0)
            {
                output[count++] = currentValue;
            }
            if (hold <= 0)
            {
                currentState = State::Decay;
            }
            break;
        case State::Decay:
            while (count < size && (currentValue > sustain))
            {
                if (decayShape < 0)
                    currentValue = sustain + (1.0f - sustain) * pow(decayCount, -decayShape + 1.0f);
                else
                    currentValue = sustain + (1.0f - sustain) * pow(decayCount, 1.0f / (decayShape + 1.0f));
                output[count++] = currentValue;
                decayCount = clamp(decayCount - decayRate, 0.0f, 1.0f);
            }
            if (currentValue <= sustainThreshold) {
                currentState = State::Sustain;
                currentValue = std::max(sustain, currentValue);
                transitionDelta = (sustain - currentValue) / (sampleRate * config::egTransitionTime);
            }
            break;
        case State::Sustain:
            if (!shouldRelease && freeRunning) {
                shouldRelease = true;
                break;
            }
            while (count < size) {
                if (currentValue > sustain)
                    currentValue += transitionDelta;
                output[count++] = currentValue;
            }
            break;
        case State::Release:
            previousValue = currentValue;
            while (count < size && (currentValue > config::egReleaseThreshold))
            {
                if (releaseShape < 0)
                    currentValue = releaseValue * pow(releaseCount, -releaseShape + 1.0f);
                else
                    currentValue = releaseValue * pow(releaseCount, 1.0f / (releaseShape + 1.0f));
                output[count++] = previousValue = currentValue;
                releaseCount = clamp(releaseCount - releaseRate, 0.0f, 1.0f);
            }
            if (currentValue <= config::egReleaseThreshold) {
                currentState = State::Fadeout;
                currentValue = previousValue;
                transitionDelta = -max(config::egReleaseThreshold, currentValue)
                    / (sampleRate * config::egTransitionTime);
            }
            break;
        case State::Fadeout:
            while (count < size && (currentValue += transitionDelta) > 0)
                output[count++] = currentValue;
            if (currentValue <= 0) {
                currentState = State::Done;
                currentValue = 0;
            }
            break;
        default:
            count = size;
            releaseValue = currentValue = 0.0;
            sfz::fill(output, currentValue);
            break;
        }

        if (shouldRelease)
        {
            releaseDelay = std::max(-1, releaseDelay - static_cast<int>(count));
            releaseValue = currentValue;
            releaseCount = 1;
        }

        output.remove_prefix(count);
    }

    this->currentState = currentState;
    this->currentValue = currentValue;
    this->shouldRelease = shouldRelease;
    this->releaseDelay = releaseDelay;
    this->transitionDelta = transitionDelta;
    this->attackCount = attackCount;
    this->decayCount = decayCount;
    this->releaseCount = releaseCount;
    this->releaseValue = releaseValue;

    ASSERT(!hasNanInf(output));
}

void ADSREnvelope::startRelease(int releaseDelay) noexcept
{
    shouldRelease = true;
    this->releaseDelay = releaseDelay;
}

void ADSREnvelope::cancelRelease(int delay) noexcept
{
    (void)delay;
    currentState = State::Sustain;
    shouldRelease = false;
    this->releaseDelay = -1;
}

void ADSREnvelope::setReleaseTime(Float timeInSeconds) noexcept
{
    releaseRate = secondsToExpRate(timeInSeconds);
}

}
