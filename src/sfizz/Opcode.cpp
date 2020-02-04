// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Opcode.h"
#include "StringViewHelpers.h"
#include <cctype>

sfz::Opcode::Opcode(absl::string_view inputOpcode, absl::string_view inputValue)
    : opcode(inputOpcode)
    , value(inputValue)
{
    trimInPlace(value);
    trimInPlace(opcode);
    size_t nextCharIndex { 0 };
    int parameterPosition { 0 };
    auto nextNumIndex = opcode.find_first_of("1234567890");
    while (nextNumIndex != opcode.npos) {
        const auto numLetters = nextNumIndex - nextCharIndex;
        parameterPosition += numLetters;
        lettersOnlyHash = hash(opcode.substr(nextCharIndex, numLetters), lettersOnlyHash);
        nextCharIndex = opcode.find_first_not_of("1234567890", nextNumIndex);

        uint32_t returnedValue;
        hasBackParameter = (nextCharIndex == opcode.npos);
        const auto numDigits = hasBackParameter ? opcode.npos : nextCharIndex - nextNumIndex;
        if (absl::SimpleAtoi(opcode.substr(nextNumIndex, numDigits), &returnedValue)) {
            ASSERT(returnedValue < std::numeric_limits<uint8_t>::max());
            parameterPositions.push_back(parameterPosition);
            parameters.push_back(returnedValue);
        }

        nextNumIndex = opcode.find_first_of("1234567890", nextCharIndex);
    }

    if (nextCharIndex != opcode.npos)
        lettersOnlyHash = hash(opcode.substr(nextCharIndex), lettersOnlyHash);
}

absl::optional<uint8_t> sfz::Opcode::backParameter() const noexcept
{
    if (hasBackParameter && !parameters.empty())
        return parameters.back();

    return {};
}

absl::optional<uint8_t> sfz::Opcode::firstParameter() const noexcept
{
    if (!hasBackParameter && !parameters.empty())
        return parameters.front();

    if (hasBackParameter && parameters.size() > 1)
        return parameters.front();

    return {};
}

absl::optional<uint8_t> sfz::Opcode::middleParameter() const noexcept
{
    if (!hasBackParameter && parameters.size() > 1)
        return parameters[1];

    if (hasBackParameter && parameters.size() > 2)
        return parameters[1];

    return {};
}
