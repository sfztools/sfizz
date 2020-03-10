// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

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
    if (resetEvents)
        clear();

    if (static_cast<int>(events.size()) < maxCapacity)
        events.emplace_back(timestamp, function(inputValue));
}

template <class Type>
void EventEnvelope<Type>::prepareEvents(int blockLength)
{
    if (resetEvents)
        clear();

    absl::c_stable_sort(events, [](const std::pair<int, Type>& lhs, const std::pair<int, Type>& rhs) {
        return lhs.first < rhs.first;
    });

    auto eventIt = events.begin();
    while (eventIt < events.end()) {
        if (eventIt->first >= blockLength) {
            eventIt->first = blockLength - 1;
            eventIt->second = events.back().second;
            ++eventIt;
            break;
        }

        auto nextEventIt = std::next(eventIt);
        while (nextEventIt < events.end() && eventIt->first == nextEventIt->first ) {
            eventIt->second = nextEventIt->second;
            ++nextEventIt;
        }
        ++eventIt;
    }
    events.resize(std::distance(events.begin(), eventIt));

    resetEvents = true;
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
void EventEnvelope<Type>::getBlock(absl::Span<Type> output)
{
    prepareEvents(output.size());
}

template <class Type>
void EventEnvelope<Type>::getQuantizedBlock(absl::Span<Type> output, Type)
{
    prepareEvents(output.size());
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

    auto quantize = [quantizationStep](Type value) -> Type {
        return std::round(value / quantizationStep) * quantizationStep;
    };

    const auto outputSize = static_cast<int>(output.size());
    for (auto& event : events) {
        const auto newValue = quantize(event.second);

        if (event.first > outputSize) {
            fill<Type>(output.subspan(index), currentValue);
            currentValue = newValue;
            index = outputSize;
            continue;
        }

        const auto length = event.first - index - 1;
        if (length <= 0) {
            currentValue = newValue;
            continue;
        }

        const auto difference = std::abs(newValue - currentValue);
        if (difference < quantizationStep) {
            fill<Type>(output.subspan(index, length), currentValue);
            currentValue = newValue;
            index += length;
            continue;
        }

        const auto numSteps = static_cast<int>(difference / quantizationStep);
        const auto stepLength = static_cast<int>(length / numSteps);
        for (int i = 0; i < numSteps; ++i) {
            fill<Type>(output.subspan(index, stepLength), currentValue);
            const auto delta = quantizationStep + currentValue - quantize(currentValue);
            currentValue += currentValue <= newValue ? delta : -delta;
            index += stepLength;
        }
    }

    if (index < outputSize)
        fill<Type>(output.subspan(index), currentValue);
}

template <class Type>
MultiplicativeEnvelope<Type>::MultiplicativeEnvelope()
{
    EventEnvelope<Type>::reset(1.0);
}

template <class Type>
void MultiplicativeEnvelope<Type>::getBlock(absl::Span<Type> output)
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

        const auto step = std::exp((std::log(event.second) - std::log(currentValue)) / length);
        multiplicativeRamp<Type>(output.subspan(index, length), currentValue, step);
        currentValue = event.second;
        index += length;
    }

    if (index < static_cast<int>(output.size()))
        fill<Type>(output.subspan(index), currentValue);
}

template <class Type>
void MultiplicativeEnvelope<Type>::getQuantizedBlock(absl::Span<Type> output, Type quantizationStep)
{
    EventEnvelope<Type>::getQuantizedBlock(output, quantizationStep);
    auto& events = EventEnvelope<Type>::events;
    auto& currentValue = EventEnvelope<Type>::currentValue;

    ASSERT(quantizationStep != 0.0);
    int index { 0 };

    const auto logStep = std::log(quantizationStep);
    // If we assume that a = b.q^r for b in (1, q) then
    // log a     log b
    // -----  =  -----  +  r
    // log q     log q
    // and log(b)\log(q) is between 0 and 1.
    auto quantize = [logStep](Type value) -> Type {
        return std::exp(logStep * std::round(std::log(value)/logStep));
    };

    const auto outputSize = static_cast<int>(output.size());
    for (auto& event : events) {
        const auto newValue = quantize(event.second);

        if (event.first > outputSize) {
            fill<Type>(output.subspan(index), currentValue);
            currentValue = newValue;
            index = outputSize;
            continue;
        }

        const auto length = event.first - index - 1;
        if (length <= 0) {
            currentValue = newValue;
            continue;
        }

        const auto difference = newValue > currentValue ? newValue / currentValue : currentValue / newValue;
        if (difference < quantizationStep) {
            fill<Type>(output.subspan(index, length), currentValue);
            currentValue = newValue;
            index += length;
            continue;
        }

        const auto numSteps = static_cast<int>(std::log(difference) / logStep);
        const auto stepLength = static_cast<int>(length / numSteps);
        for (int i = 0; i < numSteps; ++i) {
            fill<Type>(output.subspan(index, stepLength), currentValue);
            const auto delta = newValue > currentValue ?
                    quantize(currentValue) / currentValue * quantizationStep :
                    quantize(currentValue) / currentValue / quantizationStep ;
            currentValue *= delta;
            index += stepLength;
        }
    }

    if (index < outputSize)
        fill<Type>(output.subspan(index), currentValue);
}

}
