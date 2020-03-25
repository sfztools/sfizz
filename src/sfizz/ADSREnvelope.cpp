// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ADSREnvelope.h"
#include "Config.h"
#include "SIMDHelpers.h"
#include "MathHelpers.h"

namespace sfz {

template <class Type>
void ADSREnvelope<Type>::reset(const EGDescription& desc, const Region& region, const MidiState& state, int delay, float velocity, float sampleRate) noexcept
{
    auto secondsToSamples = [sampleRate](Type timeInSeconds) {
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    auto secondsToLinRate = [sampleRate](Type timeInSeconds) {
        timeInSeconds = std::max<Type>(timeInSeconds, config::virtuallyZero);
        return 1 / (sampleRate * timeInSeconds);
    };

    auto secondsToExpRate = [sampleRate](Type timeInSeconds) {
        if (timeInSeconds < config::virtuallyZero)
            return 0.0;
        return std::exp(-8.0 / (timeInSeconds * sampleRate));
    };

    this->delay = delay + secondsToSamples(desc.getDelay(state, velocity));
    this->attackStep = secondsToLinRate(desc.getAttack(state, velocity));
    this->decayRate = secondsToExpRate(desc.getDecay(state, velocity));
    this->releaseRate = secondsToExpRate(desc.getRelease(state, velocity));
    this->hold = secondsToSamples(desc.getHold(state, velocity));
    this->peak = 1.0;
    this->sustain =  normalizePercents(desc.getSustain(state, velocity));
    this->start = this->peak * normalizePercents(desc.getStart(state, velocity));

    releaseDelay = 0;
    shouldRelease = false;
    freeRunning = ((region.trigger == SfzTrigger::release) || (region.trigger == SfzTrigger::release_key));
    currentValue = this->start;
    currentState = State::Delay;
}

template <class Type>
Type ADSREnvelope<Type>::getNextValue() noexcept
{
    if (shouldRelease && releaseDelay-- == 0)
        currentState = State::Release;

    switch (currentState) {
    case State::Delay:
        if (delay-- > 0)
            return start;

        currentState = State::Attack;
        // fallthrough
    case State::Attack:
        currentValue += peak * attackStep;
        if (currentValue < peak)
            return currentValue;

        currentState = State::Hold;
        currentValue = peak;
        // fallthrough
    case State::Hold:
        if (hold-- > 0)
            return currentValue;

        currentState = State::Decay;
        // fallthrough
    case State::Decay:
        currentValue *= decayRate;
        if (currentValue > sustain)
            return currentValue;

        currentState = State::Sustain;
        currentValue = sustain;
        // fallthrough
    case State::Sustain:
        if (freeRunning)
            shouldRelease = true;
        return currentValue;
    case State::Release:
        currentValue *= releaseRate;
        if (currentValue > config::virtuallyZero)
            return currentValue;

        currentState = State::Done;
        currentValue = 0.0;
        // fallthrough
    default:
        return 0.0;
    }
}

template <class Type>
void ADSREnvelope<Type>::getBlock(absl::Span<Type> output) noexcept
{
    State currentState = this->currentState;
    Type currentValue = this->currentValue;

    while (!output.empty()) {
        size_t count = 0;
        size_t size = output.size();

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
            while (count < size && (currentValue += peak * attackStep) < peak)
                output[count++] = currentValue;
            if (currentValue >= peak) {
                currentValue = peak;
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
            if (currentValue <= sustain) {
                currentValue = sustain;
                currentState = State::Sustain;
            }
            break;
        case State::Sustain:
            if (freeRunning)
                shouldRelease = true;
            if (!shouldRelease) {
                count = size;
                currentValue = sustain;
                sfz::fill(output.first(count), currentValue);
            }
            else {
                if (releaseDelay > 0) {
                    count = clamp<size_t>(releaseDelay, 0, size);
                    currentValue = sustain;
                    sfz::fill(output.first(count), currentValue);
                    releaseDelay -= count;
                }
                if (releaseDelay <= 0)
                    currentState = State::Release;
            }
            break;
        case State::Release:
            while (count < size && (currentValue *= releaseRate) > config::virtuallyZero)
                output[count++] = currentValue;
            if (currentValue <= config::virtuallyZero) {
                currentValue = 0;
                currentState = State::Done;
            }
            break;
        default:
            count = size;
            currentValue = 0.0;
            sfz::fill(output, currentValue);
            break;
        }

        output.remove_prefix(count);
    }

    this->currentState = currentState;
    this->currentValue = currentValue;
}

template <class Type>
bool ADSREnvelope<Type>::isSmoothing() const noexcept
{
    return (currentState != State::Done);
}

template <class Type>
bool ADSREnvelope<Type>::isReleased() const noexcept
{
    return (currentState == State::Release);
}

template <class Type>
int ADSREnvelope<Type>::getRemainingDelay() const noexcept
{
    return delay;
}

template <class Type>
void ADSREnvelope<Type>::startRelease(int releaseDelay, bool fastRelease) noexcept
{
    shouldRelease = true;
    this->releaseDelay = releaseDelay;

    if (releaseDelay == 0)
        currentState = State::Release;

    if (fastRelease)
        this->releaseRate = 0;
}

}
