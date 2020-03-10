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
void ADSREnvelope<Type>::reset(const Region& region, const MidiState& state, int delay, uint8_t velocity, float sampleRate) noexcept
{
    auto secondsToSamples = [sampleRate](Type timeInSeconds) {
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    const auto ccArray = state.getCCArray();
    this->delay = delay + secondsToSamples(region.amplitudeEG.getDelay(ccArray, velocity));
    this->attack = secondsToSamples(region.amplitudeEG.getAttack(ccArray, velocity));
    this->decay = secondsToSamples(region.amplitudeEG.getDecay(ccArray, velocity));
    this->release = secondsToSamples(region.amplitudeEG.getRelease(ccArray, velocity));
    this->hold = secondsToSamples(region.amplitudeEG.getHold(ccArray, velocity));
    this->peak = 1.0;
    this->sustain =  normalizePercents(region.amplitudeEG.getSustain(ccArray, velocity));
    this->start = this->peak * normalizePercents(region.amplitudeEG.getStart(ccArray, velocity));

    releaseDelay = 0;
    shouldRelease = false;
    freeRunning = ((region.trigger == SfzTrigger::release) || (region.trigger == SfzTrigger::release_key));
    step = 0.0;
    currentValue = this->start;
    currentState = State::Delay;
}

template <class Type>
Type ADSREnvelope<Type>::getNextValue() noexcept
{
    if (shouldRelease && releaseDelay-- == 0) {
        currentState = State::Release;
        if (currentValue > config::virtuallyZero)
            step = std::exp((std::log(config::virtuallyZero) - std::log(currentValue + config::virtuallyZero)) / (release > 0 ? release : 1));
        else
            step = 1;
    }

    switch (currentState) {
    case State::Delay:
        if (delay-- > 0)
            return start;

        currentState = State::Attack;
        step = (peak - currentValue) / (attack > 0 ? attack : 1);
        // fallthrough
    case State::Attack:
        if (attack-- > 0) {
            currentValue += step;
            return currentValue;
        }

        currentState = State::Hold;
        currentValue = peak;
        // fallthrough
    case State::Hold:
        if (hold-- > 0)
            return currentValue;

        step = std::exp(std::log(sustain + config::virtuallyZero) / (decay > 0 ? decay : 1));
        currentState = State::Decay;
        // fallthrough
    case State::Decay:
        if (decay-- > 0) {
            currentValue *= step;
            return currentValue;
        }

        currentState = State::Sustain;
        currentValue = sustain;
        // fallthrough
    case State::Sustain:
        if (freeRunning)
            shouldRelease = true;
        return currentValue;
    case State::Release:
        if (release-- > 0) {
            currentValue *= step;
            return currentValue;
        }

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
    auto originalSpan = output;
    auto remainingSamples = static_cast<int>(output.size());
    int length;
    switch (currentState) {
    case State::Delay:
        length = min(remainingSamples, delay);
        fill<Type>(output, currentValue);
        output.remove_prefix(length);
        remainingSamples -= length;
        delay -= length;
        if (remainingSamples == 0)
            break;

        currentState = State::Attack;
        step = (peak - start) / (attack > 0 ? attack : 1);
        // fallthrough
    case State::Attack:
        length = min(remainingSamples, attack);
        currentValue = linearRamp<Type>(output, currentValue, step);
        output.remove_prefix(length);
        remainingSamples -= length;
        attack -= length;
        if (remainingSamples == 0)
            break;

        currentValue = peak;
        currentState = State::Hold;
        // fallthrough
    case State::Hold:
        length = min(remainingSamples, hold);
        fill<Type>(output, currentValue);
        output.remove_prefix(length);
        remainingSamples -= length;
        hold -= length;
        if (remainingSamples == 0)
            break;

        step = std::exp(std::log(sustain + config::virtuallyZero) / (decay > 0 ? decay : 1));
        currentState = State::Decay;
        // fallthrough
    case State::Decay:
        length = min(remainingSamples, decay);
        currentValue = multiplicativeRamp<Type>(output, currentValue, step);
        output.remove_prefix(length);
        remainingSamples -= length;
        decay -= length;
        if (remainingSamples == 0)
            break;

        currentValue = sustain;
        currentState = State::Sustain;
        // fallthrough
    case State::Sustain:
        if (freeRunning)
            shouldRelease = true;
        break;
    case State::Release:
        length = min(remainingSamples, release);
        currentValue = multiplicativeRamp<Type>(output, currentValue, step);
        output.remove_prefix(length);
        remainingSamples -= length;
        release -= length;
        if (remainingSamples == 0)
            break;

        currentValue = 0.0;
        currentState = State::Done;
        // fallthrough
    case State::Done:
        // fallthrough
    default:
        break;
    }
    fill<Type>(output, currentValue);

    if (shouldRelease) {
        remainingSamples = static_cast<int>(originalSpan.size());
        if (releaseDelay > remainingSamples)
        {
            releaseDelay -= remainingSamples;
            return;
        }

        originalSpan.remove_prefix(releaseDelay);
        if (originalSpan.size() > 0)
            currentValue = originalSpan.front();
        if (currentValue > config::virtuallyZero)
            step = std::exp((std::log(config::virtuallyZero) - std::log(currentValue)) / (release > 0 ? release : 1));
        else
            step = 1;
        remainingSamples -= releaseDelay;
        length = min(remainingSamples, release);
        currentState = State::Release;
        currentValue = multiplicativeRamp<Type>(originalSpan, currentValue, step);
        originalSpan.remove_prefix(length);
        release -= length;

        if (release == 0) {
            currentValue = 0.0;
            currentState = State::Done;
            fill<Type>(originalSpan, 0.0);
        }
    }
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
        this->release = 0;
}

}
