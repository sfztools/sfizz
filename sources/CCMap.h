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
#include "LeakDetector.h"
#include <map>

namespace sfz {
template <class ValueType>
class CCMap {
public:
    CCMap() = delete;
    CCMap(const ValueType& defaultValue)
        : defaultValue(defaultValue)
    {
    }
    CCMap(CCMap&&) = default;
    CCMap(const CCMap&) = default;
    ~CCMap() = default;

    const ValueType& getWithDefault(int index) const noexcept
    {
        auto it = container.find(index);
        if (it == container.end()) {
            return defaultValue;
        } else {
            return it->second;
        }
    }

    ValueType& operator[](const int& key) noexcept
    {
        if (!contains(key))
            container.emplace(key, defaultValue);
        return container.operator[](key);
    }

    inline bool empty() const { return container.empty(); }
    const ValueType& at(int index) const { return container.at(index); }
    bool contains(int index) const noexcept { return container.find(index) != container.end(); }
    typename std::map<int, ValueType>::iterator begin() { return container.begin(); }
    typename std::map<int, ValueType>::iterator end() { return container.end(); }
private:
    const ValueType defaultValue;
    std::map<int, ValueType> container;
    LEAK_DETECTOR(CCMap);
};
}