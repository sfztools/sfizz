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

#pragma once
#include "Globals.h"
#include "LeakDetector.h"
#include <absl/types/span.h>
#include <functional>
#include <type_traits>

namespace sfz {

template <class Type>
class LinearEnvelope {
public:
    LinearEnvelope();
    LinearEnvelope(int maxCapacity, std::function<Type(Type)> function);
    void setMaxCapacity(int maxCapacity);
    void setFunction(std::function<Type(Type)> function);
    void registerEvent(int timestamp, Type inputValue);
    void clear();
    void reset(Type value = 0.0);
    void getBlock(absl::Span<Type> output);

private:
    std::function<Type(Type)> function { [](Type input) { return input; } };
    static_assert(std::is_arithmetic<Type>::value);
    std::vector<std::pair<int, Type>> events;
    int maxCapacity { config::defaultSamplesPerBlock };
    Type currentValue { 0.0 };
    LEAK_DETECTOR(LinearEnvelope);
};

}