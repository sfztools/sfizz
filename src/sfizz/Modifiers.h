#pragma once
#include <numeric>
#include <array>
#include <vector>

namespace sfz {

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

template <class T>
class ModifierVector : public std::vector<T> {
public:
    T& operator[](sfz::Mod idx) { return this->std::vector<T>::operator[](static_cast<size_t>(idx)); }
    const T& operator[](sfz::Mod idx) const { return this->std::vector<T>::operator[](static_cast<size_t>(idx)); }
};

template <class T>
class ModifierArray : public std::array<T, (size_t)Mod::sentinel> {
public:
    T& operator[](sfz::Mod idx) { return this->std::array<T, (size_t)Mod::sentinel>::operator[](static_cast<size_t>(idx)); }
    const T& operator[](sfz::Mod idx) const { return this->std::array<T, (size_t)Mod::sentinel>::operator[](static_cast<size_t>(idx)); }
};

/**
 * @brief Helper for iterating over all possible modifiers.
 * Should fail at compile time if you update the modifiers but not this.
 *
 */
static const ModifierArray<Mod> allModifiers = {{
    Mod::amplitude,
    Mod::pan,
    Mod::width,
    Mod::position,
    Mod::pitch,
    Mod::volume
}};

}
