// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Defaults.h"
#include "LeakDetector.h"
#include "Range.h"
#include "SfzHelpers.h"
#include "StringViewHelpers.h"
#include "absl/types/optional.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include <vector>
#include <type_traits>
#include <iosfwd>

// charconv support is still sketchy with clang/gcc so we use abseil's numbers
#include "absl/strings/numbers.h"

namespace sfz {
/**
 * @brief A category which an opcode may belong to.
 */
enum OpcodeCategory {
    //! An ordinary opcode
    kOpcodeNormal,
    //! A region opcode which matches *_onccN or *_ccN
    kOpcodeOnCcN,
    //! A region opcode which matches *_curveccN
    kOpcodeCurveCcN,
    //! A region opcode which matches *_stepccN
    kOpcodeStepCcN,
    //! A region opcode which matches *_smoothccN
    kOpcodeSmoothCcN,
};

/**
 * @brief A scope where an opcode may appear.
 */
enum OpcodeScope {
    //! unknown scope or other
    kOpcodeScopeGeneric,
    //! global scope
    kOpcodeScopeGlobal,
    //! control scope
    kOpcodeScopeControl,
    //! Master scope
    kOpcodeScopeMaster,
    //! group scope
    kOpcodeScopeGroup,
    //! region scope
    kOpcodeScopeRegion,
    //! effect scope
    kOpcodeScopeEffect,
};

/**
 * @brief Opcode description class. The class parses the parameters
 * of the opcode on construction.
 *
 */
struct Opcode {
    Opcode() = delete;
    Opcode(absl::string_view inputOpcode, absl::string_view inputValue);
    std::string opcode {};
    std::string value {};
    uint64_t lettersOnlyHash { Fnv1aBasis };
    // This is to handle the integer parameters of some opcodes
    std::vector<uint16_t> parameters;
    OpcodeCategory category;

    /*
     * @brief Normalize in order to make the ampersand-name unique, and
     * facilitate subsequent processing.
     *
     * @param scope scope where this opcode appears
     * @return normalized opcode
     */
    Opcode cleanUp(OpcodeScope scope) const;

    /*
     * @brief Get the derived opcode name to convert it to another category.
     *
     * @param newCategory category to convert to
     * @param number optional CC number, needed if destination is CC and source is not
     * @return derived opcode name
     */
    std::string getDerivedName(OpcodeCategory newCategory, unsigned number = ~0u) const;

    /**
     * @brief Get whether the opcode categorizes as `ccN` of any kind.
     * @return true if `ccN`, otherwise false
     */
    bool isAnyCcN() const
    {
        return category == kOpcodeOnCcN || category == kOpcodeCurveCcN ||
            category == kOpcodeStepCcN || category == kOpcodeSmoothCcN;
    }

private:
    static OpcodeCategory identifyCategory(absl::string_view name);
    LEAK_DETECTOR(Opcode);
};

/**
 * @brief Convert a note in string to its equivalent midi note number
 *
 * @param value
 * @return absl::optional<uint8_t>
 */
absl::optional<uint8_t> readNoteValue(absl::string_view value);

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
template <typename ValueType, absl::enable_if_t<std::is_integral<ValueType>::value, int> = 0>
inline absl::optional<ValueType> readOpcode(absl::string_view value, const Range<ValueType>& validRange)
{
    size_t numberEnd = 0;

    if (numberEnd < value.size() && (value[numberEnd] == '+' || value[numberEnd] == '-'))
        ++numberEnd;
    while (numberEnd < value.size() && absl::ascii_isdigit(value[numberEnd]))
        ++numberEnd;

    value = value.substr(0, numberEnd);

    int64_t returnedValue;
    if (!absl::SimpleAtoi(value, &returnedValue))
            return absl::nullopt;

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
template <typename ValueType, absl::enable_if_t<std::is_floating_point<ValueType>::value, int> = 0>
inline absl::optional<ValueType> readOpcode(absl::string_view value, const Range<ValueType>& validRange)
{
    size_t numberEnd = 0;

    if (numberEnd < value.size() && (value[numberEnd] == '+' || value[numberEnd] == '-'))
        ++numberEnd;
    while (numberEnd < value.size() && absl::ascii_isdigit(value[numberEnd]))
        ++numberEnd;

    if (numberEnd < value.size() && value[numberEnd] == '.') {
        ++numberEnd;
        while (numberEnd < value.size() && absl::ascii_isdigit(value[numberEnd]))
            ++numberEnd;
    }

    value = value.substr(0, numberEnd);

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
inline void setCCPairFromOpcode(const Opcode& opcode, absl::optional<CCData<ValueType>>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (value && Default::ccNumberRange.containsWithEnd(opcode.parameters.back()))
        target = { opcode.parameters.back(), *value };
    else
        target = {};
}

}

std::ostream &operator<<(std::ostream &os, const sfz::Opcode &opcode);
