// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Opcode.h"
#include "StringViewHelpers.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include <cctype>

sfz::Opcode::Opcode(absl::string_view inputOpcode, absl::string_view inputValue)
    : opcode(trim(inputOpcode))
    , value(trim(inputValue))
    , category(identifyCategory(inputOpcode))
{
    size_t nextCharIndex { 0 };
    int parameterPosition { 0 };
    auto nextNumIndex = opcode.find_first_of("1234567890");
    while (nextNumIndex != opcode.npos) {
        const auto numLetters = nextNumIndex - nextCharIndex;
        parameterPosition += numLetters;
        lettersOnlyHash = hashNoAmpersand(opcode.substr(nextCharIndex, numLetters), lettersOnlyHash);
        nextCharIndex = opcode.find_first_not_of("1234567890", nextNumIndex);

        uint32_t returnedValue;
        const auto numDigits = (nextCharIndex == opcode.npos) ? opcode.npos : nextCharIndex - nextNumIndex;
        if (absl::SimpleAtoi(opcode.substr(nextNumIndex, numDigits), &returnedValue)) {
            lettersOnlyHash = hash("&", lettersOnlyHash);
            parameters.push_back(returnedValue);
        }

        nextNumIndex = opcode.find_first_of("1234567890", nextCharIndex);
    }

    if (nextCharIndex != opcode.npos)
        lettersOnlyHash = hashNoAmpersand(opcode.substr(nextCharIndex), lettersOnlyHash);
}

static absl::string_view extractBackInteger(absl::string_view opcodeName)
{
    size_t n = opcodeName.size();
    size_t i = n;
    while (i > 0 && absl::ascii_isdigit(opcodeName[i - 1])) --i;
    return opcodeName.substr(i);
}

sfz::OpcodeCategory sfz::Opcode::identifyCategory(absl::string_view name)
{
    sfz::OpcodeCategory category = kOpcodeNormal;

    if (!name.empty() && absl::ascii_isdigit(name.back())) {
        absl::string_view part = name;
        part.remove_suffix(extractBackInteger(name).size());
        if (absl::EndsWith(part, "_oncc") || absl::EndsWith(part, "_cc"))
            category = kOpcodeOnCcN;
        else if (absl::EndsWith(part, "_curvecc"))
            category = kOpcodeCurveCcN;
        else if (absl::EndsWith(part, "_stepcc"))
            category = kOpcodeStepCcN;
        else if (absl::EndsWith(part, "_smoothcc"))
            category = kOpcodeSmoothCcN;
    }

    return category;
}
