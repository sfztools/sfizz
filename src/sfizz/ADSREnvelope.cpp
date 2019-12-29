// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "ADSREnvelope.h"
#include "Config.h"
#include "SIMDHelpers.h"
#include "MathHelpers.h"

namespace sfz {

template <class Type>
void ADSREnvelope<Type>::reset(int attack, int release, Type sustain, int delay, int decay, int hold, Type start, Type depth) noexcept
{
    ASSERT(start <= 1.0f);
    ASSERT(sustain <= 1.0f);

    sustain = clamp<Type>(sustain, 0.0, 1.0);
    start = clamp<Type>(start, 0.0, 1.0);

    currentState = State::Done;
    this->delay = delay;
    this->attack = attack;
    this->decay = decay;
    this->release = release;
    this->hold = hold;
    this->start = depth * start;
    this->sustain = depth * sustain;
    this->peak = depth;
    releaseDelay = 0;
    shouldRelease = false;
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
        step = (1.0 - currentValue) / (attack > 0 ? attack : 1);
        [[fallthrough]];
    case State::Attack:
        if (attack-- > 0) {
            currentValue += step;
            return currentValue;
        }

        currentState = State::Hold;
        currentValue = 1.0;
        [[fallthrough]];
    case State::Hold:
        if (hold-- > 0)
            return currentValue;

        step = std::exp(std::log(sustain + config::virtuallyZero) / (decay > 0 ? decay : 1));
        currentState = State::Decay;
        [[fallthrough]];
    case State::Decay:
        if (decay-- > 0) {
            currentValue *= step;
            return currentValue;
        }

        currentState = State::Sustain;
        currentValue = sustain;
        [[fallthrough]];
    case State::Sustain:
        return currentValue;
    case State::Release:
        if (release-- > 0) {
            currentValue *= step;
            return currentValue;
        }

        currentState = State::Done;
        currentValue = 0.0;
        [[fallthrough]];
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
        [[fallthrough]];
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
        [[fallthrough]];
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
        [[fallthrough]];
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
        [[fallthrough]];
    case State::Sustain:
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
        [[fallthrough]];
    case State::Done:
        [[fallthrough]];
    default:
        break;
    }
    fill<Type>(output, currentValue);

    if (shouldRelease) {
        remainingSamples = static_cast<int>(originalSpan.size());
        if (releaseDelay > remainingSamples) {
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
bool ADSREnvelope<Type>::isSmoothing() noexcept
{
    return currentState != State::Done;
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

    if (fastRelease)
        this->release = 0;
}

}
