// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

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
