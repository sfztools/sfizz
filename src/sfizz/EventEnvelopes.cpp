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

#include "EventEnvelopes.h"
#include "SIMDHelpers.h"
#include "MathHelpers.h"
#include <absl/algorithm/container.h>

namespace sfz {

template <class Type>
EventEnvelope<Type>::EventEnvelope()
{
    setMaxCapacity(maxCapacity);
}

template <class Type>
EventEnvelope<Type>::EventEnvelope(int maxCapacity, std::function<Type(Type)> function)
{
    setMaxCapacity(maxCapacity);
    setFunction(function);
}

template <class Type>
void EventEnvelope<Type>::setMaxCapacity(int maxCapacity)
{
    events.reserve(maxCapacity);
    this->maxCapacity = maxCapacity;
}

template <class Type>
void EventEnvelope<Type>::setFunction(std::function<Type(Type)> function)
{
    this->function = function;
}

template <class Type>
void EventEnvelope<Type>::registerEvent(int timestamp, Type inputValue)
{
    
    if (static_cast<int>(events.size()) < maxCapacity)
        events.emplace_back(timestamp, function(inputValue));
}

template <class Type>
void EventEnvelope<Type>::prepareEvents()
{
    if (resetEvents) {
        if (!events.empty())
            currentValue = events.back().second;
        
        clear();
    } else {
        absl::c_sort(events, [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
        resetEvents = true;
    }
}

template <class Type>
void EventEnvelope<Type>::clear()
{
    events.clear();
    resetEvents = false;
}

template <class Type>
void EventEnvelope<Type>::reset(Type value)
{
    clear();
    currentValue = function(value);
    resetEvents = false;
}

template <class Type>
void EventEnvelope<Type>::getBlock(absl::Span<Type>)
{
    prepareEvents();
}

template <class Type>
void EventEnvelope<Type>::getQuantizedBlock(absl::Span<Type>, Type)
{
    prepareEvents();
}

template <class Type>
void LinearEnvelope<Type>::getBlock(absl::Span<Type> output)
{
    EventEnvelope<Type>::getBlock(output);
    auto& events = EventEnvelope<Type>::events;
    auto& currentValue = EventEnvelope<Type>::currentValue;

    int index { 0 };
    for (auto& event : events) {
        const auto length = min(event.first, static_cast<int>(output.size())) - index;
        if (length == 0) {
            currentValue = event.second;
            continue;
        }

        const auto step = (event.second - currentValue) / length;
        currentValue = linearRamp<Type>(output.subspan(index, length), currentValue, step);
        index += length;
    }

    if (index < static_cast<int>(output.size()))
        fill<Type>(output.subspan(index), currentValue);
}

template <class Type>
void LinearEnvelope<Type>::getQuantizedBlock(absl::Span<Type> output, Type quantizationStep)
{
    EventEnvelope<Type>::getQuantizedBlock(output, quantizationStep);
    auto& events = EventEnvelope<Type>::events;
    auto& currentValue = EventEnvelope<Type>::currentValue;
    
    ASSERT(quantizationStep != 0.0);
    int index { 0 };

    const auto halfStep = quantizationStep / 2;
    auto quantize = [quantizationStep, halfStep](Type value) -> Type {
        return static_cast<int>((value + halfStep) / quantizationStep) * quantizationStep;
    };

    currentValue = quantize(currentValue);
    const auto outputSize = static_cast<int>(output.size());
    for (auto& event : events) {
        const auto newValue = quantize(event.second);

        if (event.first > outputSize) {
            fill<Type>(output.subspan(index), currentValue);
            currentValue = newValue;
            index = outputSize;
            break;
        }

        const auto length = event.first - index - 1;
        if (length <= 0) {
            currentValue = newValue;
            continue;
        }

        const int numSteps = abs(newValue - currentValue) / quantizationStep;
        const auto stepLength = static_cast<int>(length / numSteps);
        for (int i = 0; i < numSteps; ++i) {
            fill<Type>(output.subspan(index, stepLength), currentValue);
            currentValue += currentValue <= newValue ? quantizationStep : -quantizationStep;
            index += stepLength;
        }
    }

    if (index < outputSize)
        fill<Type>(output.subspan(index), currentValue);
}

}
