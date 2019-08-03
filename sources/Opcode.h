#pragma once
#include "Helpers.h"
#include "SfzHelpers.h"
#include "Defaults.h"
#include "Range.h"
#include <string_view>
#include <optional>

// charconv support is still sketchy with clang/gcc so we use abseil's numbers
#include "absl/strings/numbers.h"

namespace sfz
{
struct Opcode
{
    Opcode() = delete;
    Opcode(std::string_view inputOpcode, std::string_view inputValue);
    std::string_view opcode{};
    std::string_view value{};	
    // This is to handle the integer parameter of some opcodes
    std::optional<uint8_t> parameter;
};

template<class ValueType>
inline std::optional<ValueType> readOpcode(std::string_view value, const Range<ValueType>& validRange)
{
    if constexpr(std::is_integral<ValueType>::value)
    {
        int64_t returnedValue;
        if (!absl::SimpleAtoi(value, &returnedValue))
            return {};

        if (returnedValue > std::numeric_limits<ValueType>::max())
            returnedValue = std::numeric_limits<ValueType>::max();
        if (returnedValue < std::numeric_limits<ValueType>::min())
            returnedValue = std::numeric_limits<ValueType>::min();

        return validRange.clamp(static_cast<ValueType>(returnedValue));
    }
    else
    {
        float returnedValue;
        if (!absl::SimpleAtof(value, &returnedValue))
            return std::nullopt;

        return validRange.clamp(returnedValue);
    }
}

template<class ValueType>
inline void setValueFromOpcode(const Opcode& opcode, ValueType& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

template<class ValueType>
inline void setValueFromOpcode(const Opcode& opcode, std::optional<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

template<class ValueType>
inline void setRangeEndFromOpcode(const Opcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setEnd(*value);
}

template<class ValueType>
inline void setRangeStartFromOpcode(const Opcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setStart(*value);
}

template<class ValueType>
inline void setCCPairFromOpcode(const Opcode& opcode, std::optional<CCValuePair>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (value && opcode.parameter &&  Default::ccRange.containsWithEnd(*opcode.parameter))
        target = std::make_pair(*opcode.parameter, *value);
    else
        target = {};
}

}
