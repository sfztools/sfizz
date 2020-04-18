// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <numeric>
#include <array>
#include <vector>

namespace sfz {

/**
 * @brief Base modifier class
 *
 */
struct Modifier {
    float value { 0.0f };
    float step { 0.0f };
    uint8_t curve { 0 };
    uint8_t smooth { 0 };
    static_assert(config::maxCurves - 1 <= std::numeric_limits<decltype(curve)>::max(), "The curve type in the Modifier struct cannot support the required number of curves");
};

enum class Mod : size_t {
    amplitude = 0,
    pan,
    width,
    position,
    pitch,
    volume,
    sentinel
};

/**
 * @brief Vectors of elements indexed on modifiers with casting and iterators
 *
 * @tparam T
 */
template <class T>
class ModifierVector : public std::vector<T> {
public:
    T& operator[](sfz::Mod idx) { return this->std::vector<T>::operator[](static_cast<size_t>(idx)); }
    const T& operator[](sfz::Mod idx) const { return this->std::vector<T>::operator[](static_cast<size_t>(idx)); }
};

/**
 * @brief Array of elements indexed on modifiers with casting and iterators
 *
 * @tparam T
 */
template <class T>
class ModifierArray {
public:
    using ContainerType = typename std::array<T, (size_t)Mod::sentinel>;
    using iterator = typename ContainerType::iterator;
    using const_iterator = typename ContainerType::const_iterator;
    ModifierArray() = default;
    ModifierArray(T val)
    {
        std::fill(underlying.begin(), underlying.end(), val);
    }
    ModifierArray(std::array<T, (size_t)Mod::sentinel>&& array) : underlying(array) {}
    T& operator[](sfz::Mod idx) { return underlying.operator[](static_cast<size_t>(idx)); }
    const T& operator[](sfz::Mod idx) const { return underlying.operator[](static_cast<size_t>(idx)); }
    iterator begin() { return underlying.begin(); }
    iterator end() { return underlying.end(); }
    const_iterator begin() const { return underlying.begin(); }
    const_iterator end() const { return underlying.end(); }
private:
    ContainerType underlying {};
};

/**
 * @brief Helper for iterating over all possible modifiers.
 * Should fail at compile time if you update the modifiers but not this.
 *
 */
static const ModifierArray<Mod> allModifiers {{
    Mod::amplitude,
    Mod::pan,
    Mod::width,
    Mod::position,
    Mod::pitch,
    Mod::volume
}};

}
