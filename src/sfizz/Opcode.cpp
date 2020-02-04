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
    size_t firstCharIndex { 0 };
    auto firstNumIndex = opcode.find_first_of("1234567890");
    while (firstNumIndex != opcode.npos) {
        lettersOnlyHash = hash(opcode.substr(firstCharIndex, firstNumIndex - firstCharIndex), lettersOnlyHash);
        firstCharIndex = opcode.find_first_not_of("1234567890", firstNumIndex);

        uint32_t returnedValue;
        if (firstCharIndex == absl::string_view::npos) {
            if (absl::SimpleAtoi(opcode.substr(firstNumIndex), &returnedValue)) {
                ASSERT(returnedValue < std::numeric_limits<uint8_t>::max());
                backParameter = static_cast<uint8_t>(returnedValue);
                break;
            }
        } else {
            if (absl::SimpleAtoi(opcode.substr(firstNumIndex, firstCharIndex - firstNumIndex), &returnedValue)) {
                ASSERT(returnedValue < std::numeric_limits<uint8_t>::max());
                parameters.push_back(static_cast<uint8_t>(returnedValue));
            }
        }
        firstNumIndex = opcode.find_first_of("1234567890", firstCharIndex);
    }

    if (firstCharIndex != opcode.npos)
        lettersOnlyHash = hash(opcode.substr(firstCharIndex), lettersOnlyHash);

}
