// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Opcode.h"
#include "StringViewHelpers.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include <iostream>
#include <cctype>
#include <cassert>

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

std::string sfz::Opcode::getDerivedName(sfz::OpcodeCategory newCategory, unsigned number) const
{
    std::string derivedName(opcode);

    switch (category) {
    case kOpcodeNormal:
        break;
    case kOpcodeOnCcN:
    case kOpcodeCurveCcN:
    case kOpcodeStepCcN:
    case kOpcodeSmoothCcN:
        {
            // when the input is cc, first delete the suffix `_*cc`
            size_t pos = opcode.rfind('_');
            assert(pos != opcode.npos);
            derivedName.resize(pos);
        }
        break;
    }

    // helper to extract the cc number optionally if the next part needs it
    auto ccNumberSuffix = [this, number]() -> std::string {
        return (number != ~0u) ? std::to_string(number) :
            std::string(extractBackInteger(opcode));
    };

    switch (newCategory) {
    case kOpcodeNormal:
        break;
    case kOpcodeOnCcN:
        absl::StrAppend(&derivedName, "_oncc", ccNumberSuffix());
        break;
    case kOpcodeCurveCcN:
        absl::StrAppend(&derivedName, "_curvecc", ccNumberSuffix());
        break;
    case kOpcodeStepCcN:
        absl::StrAppend(&derivedName, "_stepcc", ccNumberSuffix());
        break;
    case kOpcodeSmoothCcN:
        absl::StrAppend(&derivedName, "_smoothcc", ccNumberSuffix());
        break;
    }

    return derivedName;
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

absl::optional<uint8_t> sfz::readNoteValue(absl::string_view value)
{
    char noteLetter = absl::ascii_tolower(value.empty() ? '\0' : value.front());
    value.remove_prefix(1);
    if (noteLetter < 'a' || noteLetter > 'g')
        return {};

    constexpr int offsetsABCDEFG[] = { 9, 11, 0, 2, 4, 5, 7 };
    int noteNumber = offsetsABCDEFG[noteLetter - 'a'];

    ///
    absl::string_view validSharpLetters = "cdfga";
    absl::string_view validFlatLetters = "degab";

    ///
    char sharpOrFlatLetter = absl::ascii_tolower(value.empty() ? '\0' : value.front());
    if (sharpOrFlatLetter == '#') {
        if (validSharpLetters.find(noteLetter) == absl::string_view::npos)
            return {};
        ++noteNumber;
        value.remove_prefix(1);
    }
    else if (sharpOrFlatLetter == 'b') {
        if (validFlatLetters.find(noteLetter) == absl::string_view::npos)
            return {};
        --noteNumber;
        value.remove_prefix(1);
    }

    int octaveNumber;
    if (!absl::SimpleAtoi(value, &octaveNumber))
        return {};

    noteNumber += (octaveNumber + 1) * 12;

    if (noteNumber < 0 || noteNumber >= 128)
        return {};

    return static_cast<uint8_t>(noteNumber);
}

std::ostream &operator<<(std::ostream &os, const sfz::Opcode &opcode)
{
    return os << opcode.opcode << '=' << '"' << opcode.value << '"';
}
