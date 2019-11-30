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
#include "Defaults.h"
#include "LeakDetector.h"
#include "Range.h"
#include "SfzHelpers.h"
#include "StringViewHelpers.h"
#include <absl/types/optional.h>
#include <string_view>
#include <type_traits>

// charconv support is still sketchy with clang/gcc so we use abseil's numbers
#include "absl/strings/numbers.h"

namespace sfz {
/**
 * @brief Opcode description class; should be very lightweight to use
 * and move around. The class parses the parameters of the opcode
 * on construction.
 *
 */
struct Opcode {
    Opcode() = delete;
    Opcode(absl::string_view inputOpcode, absl::string_view inputValue);
    absl::string_view opcode {};
    absl::string_view value {};
    // This is to handle the integer parameter of some opcodes
    absl::optional<uint8_t> parameter;
    LEAK_DETECTOR(Opcode);
};

/**
 * @brief Read a value from the sfz file and cast it to the destination parameter along
 * with a proper clamping into range if needed. This particular template version acts on
 * integral target types, but can accept floats as an input.
 *
 * @tparam ValueType the target casting type
 * @param value the string value to be read and stored
 * @param validRange the range of admitted values
 * @return absl::optional<ValueType> the cast value, or null
 */
template <typename ValueType, std::enable_if_t<std::is_integral<ValueType>::value, int> = 0>
inline absl::optional<ValueType> readOpcode(absl::string_view value, const Range<ValueType>& validRange)
{
        int64_t returnedValue;
        if (!absl::SimpleAtoi(value, &returnedValue)) {
            float floatValue;
            if (!absl::SimpleAtof(value, &floatValue))
                return {};
            returnedValue = static_cast<int64_t>(floatValue);
        }

        if (returnedValue > std::numeric_limits<ValueType>::max())
            returnedValue = std::numeric_limits<ValueType>::max();
        if (returnedValue < std::numeric_limits<ValueType>::min())
            returnedValue = std::numeric_limits<ValueType>::min();

        return validRange.clamp(static_cast<ValueType>(returnedValue));
}

/**
 * @brief Read a value from the sfz file and cast it to the destination parameter along
 * with a proper clamping into range if needed. This particular template version acts on
 * floating types.
 *
 * @tparam ValueType the target casting type
 * @param value the string value to be read and stored
 * @param validRange the range of admitted values
 * @return absl::optional<ValueType> the cast value, or null
 */
template <typename ValueType, std::enable_if_t<std::is_floating_point<ValueType>::value, int> = 0>
inline absl::optional<ValueType> readOpcode(absl::string_view value, const Range<ValueType>& validRange)
{
    float returnedValue;
    if (!absl::SimpleAtof(value, &returnedValue))
		return absl::nullopt;

    return validRange.clamp(returnedValue);
}

/**
 * @brief Read a boolean value from the sfz file and cast it to the destination parameter.
 */
inline absl::optional<bool> readBooleanFromOpcode(const Opcode& opcode)
{
    switch (hash(opcode.value)) {
    case hash("off"):
        return false;
    case hash("on"):
        return true;
    default:
        return {};
    }
}

/**
 * @brief Set a target parameter from an opcode value, with possibly a textual note rather
 * than a number
 *
 * @tparam ValueType
 * @param opcode the source opcode
 * @param target the value to update
 * @param validRange the range of admitted values used to clamp the opcode
 */
template <class ValueType>
inline void setValueFromOpcode(const Opcode& opcode, ValueType& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

/**
 * @brief Set a target parameter from an opcode value, with possibly a textual note rather
 * than a number
 *
 * @tparam ValueType
 * @param opcode the source opcode
 * @param target the value to update
 * @param validRange the range of admitted values used to clamp the opcode
 */
template <class ValueType>
inline void setValueFromOpcode(const Opcode& opcode, absl::optional<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

/**
 * @brief Set a target end of a range from an opcode value, with possibly a textual note rather
 * than a number
 *
 * @tparam ValueType
 * @param opcode the source opcode
 * @param target the value to update
 * @param validRange the range of admitted values used to clamp the opcode
 */
template <class ValueType>
inline void setRangeEndFromOpcode(const Opcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setEnd(*value);
}

/**
 * @brief Set a target beginning of a range from an opcode value, with possibly a textual note rather
 * than a number
 *
 * @tparam ValueType
 * @param opcode the source opcode
 * @param target the value to update
 * @param validRange the range of admitted values used to clamp the opcode
 */
template <class ValueType>
inline void setRangeStartFromOpcode(const Opcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setStart(*value);
}

/**
 * @brief Set a CC modulation parameter from an opcode value.
 *
 * @tparam ValueType
 * @param opcode the source opcode
 * @param target the new CC modulation parameter
 * @param validRange the range of admitted values used to clamp the opcode
 */
template <class ValueType>
inline void setCCPairFromOpcode(const Opcode& opcode, absl::optional<CCValuePair>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (value && opcode.parameter && Default::ccRange.containsWithEnd(*opcode.parameter))
        target = std::make_pair(*opcode.parameter, *value);
    else
        target = {};
}

}
