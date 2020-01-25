// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
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
/**
 * @brief A simple map that holds ValueType elements at different indices, and can return a default one
 * if not present. Used mostly for CC modifiers in region descriptions as to store only the CC modifiers
 * that are specified in the SFZ file rather than a gazillion of dummy "disabled" modifiers. The default
 * value is set on construction.
 *
 * @tparam ValueType The type held in the map
 */
template <class ValueType>
class CCMap {
public:
    CCMap() = delete;
    /**
     * @brief Construct a new CCMap object with the specified default value.
     *
     * @param defaultValue
     */
    CCMap(const ValueType& defaultValue)
        : defaultValue(defaultValue)
    {
    }
    CCMap(CCMap&&) = default;
    CCMap(const CCMap&) = default;
    ~CCMap() = default;

    /**
     * @brief Returns the held object at the index, or a default value if not present
     *
     * @param index
     * @return const ValueType&
     */
    const ValueType& getWithDefault(int index) const noexcept
    {
        auto it = container.find(index);
        if (it == container.end()) {
            return defaultValue;
        } else {
            return it->second;
        }
    }

    /**
     * @brief Get the value at index key or emplace a new one if not present
     *
     * @param key the index of the element
     * @return ValueType&
     */
    ValueType& operator[](const int& key) noexcept
    {
        if (!contains(key))
            container.emplace(key, defaultValue);
        return container.operator[](key);
    }

    /**
     * @brief Is the container empty
     *
     * @return true
     * @return false
     */
    inline bool empty() const { return container.empty(); }
    /**
     * @brief Returns the value at index with bounds checking (and possibly exceptions)
     *
     * @param index
     * @return const ValueType&
     */
    const ValueType& at(int index) const { return container.at(index); }
    /**
     * @brief Returns true if the container containers an element at index
     *
     * @param index
     * @return true
     * @return false
     */
    bool contains(int index) const noexcept { return container.find(index) != container.end(); }
    typename std::map<int, ValueType>::iterator begin() { return container.begin(); }
    typename std::map<int, ValueType>::iterator end() { return container.end(); }
private:
    const ValueType defaultValue;
    std::map<int, ValueType> container;
    LEAK_DETECTOR(CCMap);
};
}
