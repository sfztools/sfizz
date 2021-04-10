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
        return Float(0.0);

    timeInSeconds = std::max(Float(25e-3), timeInSeconds);
    return std::exp(Float(-9.0) / (timeInSeconds * sampleRate));
};

void ADSREnvelope::reset(const EGDescription& desc, const Region& region, const MidiState& state, int delay, float velocity, float sampleRate) noexcept
{
    this->sampleRate = sampleRate;

    this->delay = delay + secondsToSamples(desc.getDelay(state, velocity));
    this->attackStep = secondsToLinRate(desc.getAttack(state, velocity));
    this->decayRate = secondsToExpRate(desc.getDecay(state, velocity));
    this->releaseRate = secondsToExpRate(desc.getRelease(state, velocity));
    this->hold = secondsToSamples(desc.getHold(state, velocity));
    this->sustain = clamp(desc.getSustain(state, velocity), 0.0f, 1.0f);
    this->start = clamp(desc.getStart(state, velocity), 0.0f, 1.0f);

    releaseDelay = 0;
    sustainThreshold = this->sustain + config::virtuallyZero;
    shouldRelease = false;
    freeRunning = (
        (this->sustain <= Float(config::sustainFreeRunningThreshold))
        || (region.loopMode == LoopMode::one_shot && region.isOscillator())
    );
    currentValue = this->start;
    currentState = State::Delay;
}

void ADSREnvelope::getBlock(absl::Span<Float> output) noexcept
{
    State currentState = this->currentState;
    Float currentValue = this->currentValue;
    bool shouldRelease = this->shouldRelease;
    int releaseDelay = this->releaseDelay;
    Float transitionDelta = this->transitionDelta;

    while (!output.empty()) {
        size_t count = 0;
        size_t size = output.size();

        if (shouldRelease && releaseDelay == 0) {
            // release takes effect this frame
            currentState = State::Release;
            releaseDelay = -1;
        } else if (shouldRelease && releaseDelay > 0) {
            // prevent computing the segment further than release point
            size = std::min<size_t>(size, releaseDelay);
        }

        Float previousValue;

        switch (currentState) {
        case State::Delay:
            while (count < size && delay-- > 0) {
                currentValue = start;
                output[count++] = currentValue;
            }
            if (delay <= 0)
                currentState = State::Attack;
            break;
        case State::Attack:
            while (count < size && (currentValue += attackStep) < 1)
                output[count++] = currentValue;
            if (currentValue >= 1) {
                currentValue = 1;
                currentState = State::Hold;
            }
            break;
        case State::Hold:
            while (count < size && hold-- > 0)
                output[count++] = currentValue;
            if (hold <= 0)
                currentState = State::Decay;
            break;
        case State::Decay:
            while (count < size && (currentValue *= decayRate) > sustain)
                output[count++] = currentValue;
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
                currentValue = std::max(sustain, currentValue + transitionDelta);
                output[count++] = currentValue;
            }
            break;
        case State::Release:
            previousValue = currentValue;
            while (count < size && (currentValue *= releaseRate) > config::egReleaseThreshold)
                output[count++] = previousValue = currentValue;
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
            currentValue = 0.0;
            sfz::fill(output, currentValue);
            break;
        }

        if (shouldRelease)
            releaseDelay = std::max(-1, releaseDelay - static_cast<int>(count));

        output.remove_prefix(count);
    }

    this->currentState = currentState;
    this->currentValue = currentValue;
    this->shouldRelease = shouldRelease;
    this->releaseDelay = releaseDelay;
    this->transitionDelta = transitionDelta;

    ASSERT(!hasNanInf(output));
}

void ADSREnvelope::startRelease(int releaseDelay) noexcept
{
    shouldRelease = true;
    this->releaseDelay = releaseDelay;
}

void ADSREnvelope::setReleaseTime(Float timeInSeconds) noexcept
{
    releaseRate = secondsToExpRate(timeInSeconds);
}

}
