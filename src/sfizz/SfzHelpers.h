// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/types/optional.h>
#include <absl/strings/string_view.h>
//#include <string>
#include <array>
#include <cmath>
#include "Config.h"
#include "MathHelpers.h"

namespace sfz
{

using SfzCCArray = std::array<uint8_t, config::numCCs>;
using CCNamePair = std::pair<uint8_t, std::string>;

template<class ValueType>
struct CCValuePair {
    int cc;
    ValueType value;
};

template<class ValueType, bool CompareValue = false>
struct CCValuePairComparator {
    bool operator()(const CCValuePair<ValueType>& valuePair, const int& cc)
    {
        return (valuePair.cc < cc);
    }

    bool operator()(const int& cc, const CCValuePair<ValueType>& valuePair)
    {
        return (cc < valuePair.cc);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.cc < rhs.cc);
    }
};

template<class ValueType>
struct CCValuePairComparator<ValueType, true> {
    bool operator()(const CCValuePair<ValueType>& valuePair, const ValueType& value)
    {
        return (valuePair.value < value);
    }

    bool operator()(const ValueType& value, const CCValuePair<ValueType>& valuePair)
    {
        return (value < valuePair.value);
    }

    bool operator()(const CCValuePair<ValueType>& lhs, const CCValuePair<ValueType>& rhs)
    {
        return (lhs.value < rhs.value);
    }
};

/**
 * @brief Converts cents to a pitch ratio
 *
 * @tparam T
 * @param cents
 * @param centsPerOctave
 * @return constexpr float
 */
template<class T>
constexpr float centsFactor(T cents, T centsPerOctave = 1200)
{
    return std::pow(2.0f, static_cast<float>(cents) / centsPerOctave);
}

/**
 * @brief Normalize a CC value between (T)0.0 and (T)1.0
 *
 * @tparam T
 * @param ccValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizeCC(T ccValue)
{
    static_assert(std::is_integral<T>::value, "Requires an integral T");
    return static_cast<float>(min(max(ccValue, static_cast<T>(0)), static_cast<T>(127))) / 127.0f;
}

/**
 * @brief Normalize a velocity between (T)0.0 and (T)1.0
 *
 * @tparam T
 * @param ccValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizeVelocity(T velocity)
{
    return normalizeCC(velocity);
}


/**
 * @brief Normalize a percentage between 0 and 1
 *
 * @tparam T
 * @param percentValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizePercents(T percentValue)
{
    return percentValue * 0.01f;
}

/**
 * @brief Normalize bends between -1 and 1. We clamp to 8191 instead of 8192 in the low end
 * to have something symmetric with respect to 0.
 *
 * @param bendValue
 * @return constexpr float
 */
constexpr float normalizeBend(float bendValue)
{
    return min(max(bendValue, -8191.0f), 8191.0f) / 8191.0f;
}

/**
 * @brief If a cc switch exists for the value, returns the value with the CC modifier, otherwise returns the value alone.
 *
 * @param ccValues
 * @param ccSwitch
 * @param value
 * @return float
 */
inline float ccSwitchedValue(const SfzCCArray& ccValues, const absl::optional<CCValuePair<float>>& ccSwitch, float value) noexcept
{
    if (ccSwitch)
        return value + ccSwitch->value * normalizeCC(ccValues[ccSwitch->cc]);
    else
        return value;
}

/**
 * @brief Convert a note in string to its equivalent midi note number
 *
 * @param value
 * @return absl::optional<uint8_t>
 */
absl::optional<uint8_t> readNoteValue(const absl::string_view& value);

/**
 * @brief From a source view, find the next sfz header and its members and
 *          return them, while updating the source by removing this header
 *          and members from the beginning. The function "consumes" the
 *          header and its members from the source if found.
 *
 * No check is made to see if the header is "valid" in the sfz sense.
 * The output parameters are set only if the method returns true.
 *
 * @param source A source view; can be updated and shortened
 * @param header An output view on the header, without the <>
 * @param members An output view on the members, untrimmed
 * @return true if a header was found
 * @return false otherwise
 */
bool findHeader(absl::string_view& source, absl::string_view& header, absl::string_view& members);
/**
 * @brief From a source view, find the next sfz member opcode and its value.
 *          Return them while updating the source by removing this opcode
 *          and value from the beginning. The function "consumes" the
 *          opcode from the source if one is found.
 *
 * No check is made to see if the opcode is "valid" in the sfz sense.
 * The output parameters are set only if the method returns true.
 *
 * @param source A source view; can be updated and shortened
 * @param opcode An output view on the opcode name
 * @param value An output view on the opcode value
 * @return true if an opcode was found
 * @return false
 */
bool findOpcode(absl::string_view& source, absl::string_view& opcode, absl::string_view& value);

/**
 * @brief Find an SFZ #define statement on a line and return the variable and value as views.
 *
 * This function assums that there is a single define per line and that the variable and value
 * are separated by whitespace.
 * The output parameters are set only if the method returns true.
 *
 * @param line The source line
 * @param variable An output view on the define variable
 * @param value An output view on the define value
 * @return true If a define was found
 * @return false
 */
bool findDefine(absl::string_view line, absl::string_view& variable, absl::string_view& value);

/**
 * @brief Find an SFZ #include statement on a line and return included path.
 *
 * This function assums that there is a single include per line and that the
 * include path is within quotes.
 * The output parameter is set only if the method returns true.
 *
 * @param line The source line
 * @param path The path, if found
 * @return true If an include was found
 * @return false
 */
bool findInclude(absl::string_view line, std::string& path);

/**
 * @brief Defines a function that modulates a base value with another one
 *
 * @tparam T
 */
template<class T, class U>
using modFunction = std::function<void(T&, U)>;

/**
 * @brief Modulation helper that adds the modifier to the base value
 *
 * @tparam T
 * @param base the base value
 * @param modifier the modifier value
 */
template<class T>
constexpr void addToBase(T& base, T modifier)
{
    base += modifier;
}

/**
 * @brief multiply a value by a factor, in cents. To be used for pitch variations.
 *
 * @param base
 * @param modifier
 */
inline void multiplyByCents(float& base, int modifier)
{
    base *= centsFactor(modifier);
}

} // namespace sfz

