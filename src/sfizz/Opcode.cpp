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

namespace sfz {

Opcode::Opcode(absl::string_view inputOpcode, absl::string_view inputValue)
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

std::string Opcode::getDerivedName(OpcodeCategory newCategory, unsigned number) const
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

OpcodeCategory Opcode::identifyCategory(absl::string_view name)
{
    OpcodeCategory category = kOpcodeNormal;

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

template <typename T>
absl::optional<T> readInt_(OpcodeSpec<T> spec, absl::string_view v)
{
    size_t numberEnd = 0;

    if (numberEnd < v.size() && (v[numberEnd] == '+' || v[numberEnd] == '-'))
        ++numberEnd;

    while (numberEnd < v.size() && absl::ascii_isdigit(v[numberEnd]))
        ++numberEnd;

    if (numberEnd == 0 && (spec.flags & kCanBeNote))
        return readNoteValue(v);

    v = v.substr(0, numberEnd);

    int64_t returnedValue;
    if (!absl::SimpleAtoi(v, &returnedValue))
        return absl::nullopt;

    if (returnedValue > static_cast<int64_t>(spec.bounds.getEnd())) {
        if (spec.flags & kEnforceUpperBound)
            return spec.bounds.getEnd();

        if (spec.flags & kIgnoreOOB)
            return {};
    }

    if (returnedValue < static_cast<int64_t>(spec.bounds.getStart())) {
        if (spec.flags & kEnforceLowerBound)
            return spec.bounds.getStart();

        if (spec.flags & kIgnoreOOB)
            return {};
    }

    T castValue = static_cast<T>(returnedValue);
    if ((castValue != returnedValue) & kIgnoreOOB)
        return {};

    return castValue;
}

#define INSTANTIATE_FOR_INTEGRAL(T)                             \
    template <>                                                 \
    absl::optional<T> Opcode::read(OpcodeSpec<T> spec) const    \
    {                                                           \
        return readInt_<T>(spec, value);                               \
    }

INSTANTIATE_FOR_INTEGRAL(uint8_t)
INSTANTIATE_FOR_INTEGRAL(uint16_t)
INSTANTIATE_FOR_INTEGRAL(uint32_t)
INSTANTIATE_FOR_INTEGRAL(int8_t)
INSTANTIATE_FOR_INTEGRAL(int16_t)
INSTANTIATE_FOR_INTEGRAL(int32_t)
INSTANTIATE_FOR_INTEGRAL(int64_t)


template <typename T>
absl::optional<T> readFloat_(OpcodeSpec<T> spec, absl::string_view v)
{
    size_t numberEnd = 0;

    if (numberEnd < v.size() && (v[numberEnd] == '+' || v[numberEnd] == '-'))
        ++numberEnd;
    while (numberEnd < v.size() && absl::ascii_isdigit(v[numberEnd]))
        ++numberEnd;

    if (numberEnd < v.size() && v[numberEnd] == '.') {
        ++numberEnd;
        while (numberEnd < v.size() && absl::ascii_isdigit(v[numberEnd]))
            ++numberEnd;
    }

    v = v.substr(0, numberEnd);

    float returnedValue;
    if (!absl::SimpleAtof(v, &returnedValue))
        return absl::nullopt;

    if (returnedValue > static_cast<int64_t>(spec.bounds.getEnd())) {
        if (spec.flags & kEnforceUpperBound)
            return spec.bounds.getEnd();

        if (spec.flags & kIgnoreOOB)
            return {};
    }

    if (returnedValue < static_cast<int64_t>(spec.bounds.getStart())) {
        if (spec.flags & kEnforceLowerBound)
            return spec.bounds.getStart();

        if (spec.flags & kIgnoreOOB)
            return {};
    }

    return returnedValue;
}

#define INSTANTIATE_FOR_FLOATING_POINT(T)                       \
    template <>                                                 \
    absl::optional<T> Opcode::read(OpcodeSpec<T> spec) const    \
    {                                                           \
        return readFloat_<T>(spec, value);                             \
    }

INSTANTIATE_FOR_FLOATING_POINT(float)
INSTANTIATE_FOR_FLOATING_POINT(double)

absl::optional<uint8_t> readNoteValue(absl::string_view value)
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
    std::pair<absl::string_view, int> flatSharpPrefixes[] = {
        {   "#", +1 },
        { u8"♯", +1 },
        {   "b", -1 },
        { u8"♭", -1 },
    };

    for (const auto& prefix : flatSharpPrefixes) {
        if (absl::StartsWith(value, prefix.first)) {
            if (prefix.second == +1) {
                if (validSharpLetters.find(noteLetter) == absl::string_view::npos)
                    return {};
            }
            else if (prefix.second == -1) {
                if (validFlatLetters.find(noteLetter) == absl::string_view::npos)
                    return {};
            }
            noteNumber += prefix.second;
            value.remove_prefix(prefix.first.size());
            break;
        }
    }

    int octaveNumber;
    if (!absl::SimpleAtoi(value, &octaveNumber))
        return {};

    noteNumber += (octaveNumber + 1) * 12;

    if (noteNumber < 0 || noteNumber >= 128)
        return {};

    return static_cast<uint8_t>(noteNumber);
}

absl::optional<bool> readBooleanFromOpcode(const Opcode& opcode)
{
    // Cakewalk-style booleans, case-insensitive
    if (absl::EqualsIgnoreCase(opcode.value, "off"))
        return false;
    if (absl::EqualsIgnoreCase(opcode.value, "on"))
        return true;

    // ARIA-style booleans? (seen in egN_dynamic=1 for example)
    // TODO check this
    const OpcodeSpec<int64_t> fullInt64 { 0, Range<int64_t>::wholeRange(), 0 };
    if (auto value = opcode.read(fullInt64))
        return *value != 0;

    return absl::nullopt;
}

template <>
absl::optional<OscillatorEnabled> Opcode::read(OpcodeSpec<OscillatorEnabled>) const
{
    auto v = readBooleanFromOpcode(*this);
    if (!v)
        return absl::nullopt;

    return *v ? OscillatorEnabled::On : OscillatorEnabled::Off;
}

template <>
absl::optional<bool> Opcode::read(OpcodeSpec<bool>) const
{
    return readBooleanFromOpcode(*this);
}

} // namespace sfz

std::ostream &operator<<(std::ostream &os, const sfz::Opcode &opcode)
{
    return os << opcode.opcode << '=' << '"' << opcode.value << '"';
}
