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

#include "LinearEnvelope.h"
#include "SIMDHelpers.h"
#include "MathHelpers.h"
#include <absl/algorithm/container.h>

namespace sfz {

template <class Type>
LinearEnvelope<Type>::LinearEnvelope()
{
    setMaxCapacity(maxCapacity);
}

template <class Type>
LinearEnvelope<Type>::LinearEnvelope(int maxCapacity, std::function<Type(Type)> function)
{
    setMaxCapacity(maxCapacity);
    setFunction(function);
}

template <class Type>
void LinearEnvelope<Type>::setMaxCapacity(int maxCapacity)
{
    events.reserve(maxCapacity);
    this->maxCapacity = maxCapacity;
}

template <class Type>
void LinearEnvelope<Type>::setFunction(std::function<Type(Type)> function)
{
    this->function = function;
}

template <class Type>
void LinearEnvelope<Type>::registerEvent(int timestamp, Type inputValue)
{
    if (static_cast<int>(events.size()) < maxCapacity)
        events.emplace_back(timestamp, function(inputValue));
}

template <class Type>
void LinearEnvelope<Type>::clear()
{
    events.clear();
}

template <class Type>
void LinearEnvelope<Type>::reset(Type value)
{
    clear();
    currentValue = function(value);
}

template <class Type>
void LinearEnvelope<Type>::getBlock(absl::Span<Type> output)
{
    absl::c_sort(events, [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
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

    clear();
}

}