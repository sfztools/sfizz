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
#include <absl/types/optional.h>
#include <absl/strings/string_view.h>
//#include <string>
#include <array>
#include <cmath>
#include "Config.h"

namespace sfz
{

using SfzCCArray = std::array<uint8_t, config::numCCs>;
using CCValuePair = std::pair<uint8_t, float> ;
using CCNamePair = std::pair<uint8_t, std::string>;

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
    return static_cast<float>(std::min(std::max(ccValue, static_cast<T>(0)), static_cast<T>(127))) / 127.0f;
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
    return std::min(std::max(static_cast<float>(percentValue), 0.0f), 100.0f) / 100.0f;
}

/**
 * @brief Normalize bends between -1 and 1. We clamp to 8191 instead of 8192 in the low end
 * to have something symmetric with respect to 0.
 *
 * @param bendValue
 * @return constexpr float
 */
constexpr float normalizeBend(int bendValue)
{
    return std::min(std::max(static_cast<float>(bendValue), -8191.0f), 8191.0f) / 8191.0f;
}

/**
 * @brief Normalize a possibly negative percentage between -1 and 1
 *
 * @tparam T
 * @param percentValue
 * @return constexpr float
 */
template<class T>
constexpr float normalizeNegativePercents(T percentValue)
{
    return std::min(std::max(static_cast<float>(percentValue), -100.0f), 100.0f) / 100.0f;
}

/**
 * @brief If a cc switch exists for the value, returns the value with the CC modifier, otherwise returns the value alone.
 *
 * @param ccValues
 * @param ccSwitch
 * @param value
 * @return float
 */
inline float ccSwitchedValue(const SfzCCArray& ccValues, const absl::optional<CCValuePair>& ccSwitch, float value) noexcept
{
    if (ccSwitch)
        return value + ccSwitch->second * normalizeCC(ccValues[ccSwitch->first]);
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

} // namespace sfz

