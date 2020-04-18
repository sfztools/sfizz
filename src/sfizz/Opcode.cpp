// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Opcode.h"
#include "StringViewHelpers.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
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

absl::optional<uint8_t> sfz::readNoteValue(const absl::string_view& value)
{
    switch(hash(value))
    {
        case hash("c-1"):   case hash("C-1"):   return (uint8_t) 0;
        case hash("c#-1"):  case hash("C#-1"):  return (uint8_t) 1;
        case hash("d-1"):   case hash("D-1"):   return (uint8_t) 2;
        case hash("d#-1"):  case hash("D#-1"):  return (uint8_t) 3;
        case hash("e-1"):   case hash("E-1"):   return (uint8_t) 4;
        case hash("f-1"):   case hash("F-1"):   return (uint8_t) 5;
        case hash("f#-1"):  case hash("F#-1"):  return (uint8_t) 6;
        case hash("g-1"):   case hash("G-1"):   return (uint8_t) 7;
        case hash("g#-1"):  case hash("G#-1"):  return (uint8_t) 8;
        case hash("a-1"):   case hash("A-1"):   return (uint8_t) 9;
        case hash("a#-1"):  case hash("A#-1"):  return (uint8_t) 10;
        case hash("b-1"):   case hash("B-1"):   return (uint8_t) 11;

        case hash("c0"):    case hash("C0"):    return (uint8_t) 12;
        case hash("c#0"):   case hash("C#0"):   return (uint8_t) 13;
        case hash("d0"):    case hash("D0"):    return (uint8_t) 14;
        case hash("d#0"):   case hash("D#0"):   return (uint8_t) 15;
        case hash("e0"):    case hash("E0"):    return (uint8_t) 16;
        case hash("f0"):    case hash("F0"):    return (uint8_t) 17;
        case hash("f#0"):   case hash("F#0"):   return (uint8_t) 18;
        case hash("g0"):    case hash("G0"):    return (uint8_t) 19;
        case hash("g#0"):   case hash("G#0"):   return (uint8_t) 20;
        case hash("a0"):    case hash("A0"):    return (uint8_t) 21;
        case hash("a#0"):   case hash("A#0"):   return (uint8_t) 22;
        case hash("b0"):    case hash("B0"):    return (uint8_t) 23;

        case hash("c1"):    case hash("C1"):    return (uint8_t) 24;
        case hash("c#1"):   case hash("C#1"):   return (uint8_t) 25;
        case hash("d1"):    case hash("D1"):    return (uint8_t) 26;
        case hash("d#1"):   case hash("D#1"):   return (uint8_t) 27;
        case hash("e1"):    case hash("E1"):    return (uint8_t) 28;
        case hash("f1"):    case hash("F1"):    return (uint8_t) 29;
        case hash("f#1"):   case hash("F#1"):   return (uint8_t) 30;
        case hash("g1"):    case hash("G1"):    return (uint8_t) 31;
        case hash("g#1"):   case hash("G#1"):   return (uint8_t) 32;
        case hash("a1"):    case hash("A1"):    return (uint8_t) 33;
        case hash("a#1"):   case hash("A#1"):   return (uint8_t) 34;
        case hash("b1"):    case hash("B1"):    return (uint8_t) 35;

        case hash("c2"):    case hash("C2"):    return (uint8_t) 36;
        case hash("c#2"):   case hash("C#2"):   return (uint8_t) 37;
        case hash("d2"):    case hash("D2"):    return (uint8_t) 38;
        case hash("d#2"):   case hash("D#2"):   return (uint8_t) 39;
        case hash("e2"):    case hash("E2"):    return (uint8_t) 40;
        case hash("f2"):    case hash("F2"):    return (uint8_t) 41;
        case hash("f#2"):   case hash("F#2"):   return (uint8_t) 42;
        case hash("g2"):    case hash("G2"):    return (uint8_t) 43;
        case hash("g#2"):   case hash("G#2"):   return (uint8_t) 44;
        case hash("a2"):    case hash("A2"):    return (uint8_t) 45;
        case hash("a#2"):   case hash("A#2"):   return (uint8_t) 46;
        case hash("b2"):    case hash("B2"):    return (uint8_t) 47;

        case hash("c3"):    case hash("C3"):    return (uint8_t) 48;
        case hash("c#3"):   case hash("C#3"):   return (uint8_t) 49;
        case hash("d3"):    case hash("D3"):    return (uint8_t) 50;
        case hash("d#3"):   case hash("D#3"):   return (uint8_t) 51;
        case hash("e3"):    case hash("E3"):    return (uint8_t) 52;
        case hash("f3"):    case hash("F3"):    return (uint8_t) 53;
        case hash("f#3"):   case hash("F#3"):   return (uint8_t) 54;
        case hash("g3"):    case hash("G3"):    return (uint8_t) 55;
        case hash("g#3"):   case hash("G#3"):   return (uint8_t) 56;
        case hash("a3"):    case hash("A3"):    return (uint8_t) 57;
        case hash("a#3"):   case hash("A#3"):   return (uint8_t) 58;
        case hash("b3"):    case hash("B3"):    return (uint8_t) 59;

        case hash("c4"):    case hash("C4"):    return (uint8_t) 60;
        case hash("c#4"):   case hash("C#4"):   return (uint8_t) 61;
        case hash("d4"):    case hash("D4"):    return (uint8_t) 62;
        case hash("d#4"):   case hash("D#4"):   return (uint8_t) 63;
        case hash("e4"):    case hash("E4"):    return (uint8_t) 64;
        case hash("f4"):    case hash("F4"):    return (uint8_t) 65;
        case hash("f#4"):   case hash("F#4"):   return (uint8_t) 66;
        case hash("g4"):    case hash("G4"):    return (uint8_t) 67;
        case hash("g#4"):   case hash("G#4"):   return (uint8_t) 68;
        case hash("a4"):    case hash("A4"):    return (uint8_t) 69;
        case hash("a#4"):   case hash("A#4"):   return (uint8_t) 70;
        case hash("b4"):    case hash("B4"):    return (uint8_t) 71;

        case hash("c5"):    case hash("C5"):    return (uint8_t) 72;
        case hash("c#5"):   case hash("C#5"):   return (uint8_t) 73;
        case hash("d5"):    case hash("D5"):    return (uint8_t) 74;
        case hash("d#5"):   case hash("D#5"):   return (uint8_t) 75;
        case hash("e5"):    case hash("E5"):    return (uint8_t) 76;
        case hash("f5"):    case hash("F5"):    return (uint8_t) 77;
        case hash("f#5"):   case hash("F#5"):   return (uint8_t) 78;
        case hash("g5"):    case hash("G5"):    return (uint8_t) 79;
        case hash("g#5"):   case hash("G#5"):   return (uint8_t) 80;
        case hash("a5"):    case hash("A5"):    return (uint8_t) 81;
        case hash("a#5"):   case hash("A#5"):   return (uint8_t) 82;
        case hash("b5"):    case hash("B5"):    return (uint8_t) 83;

        case hash("c6"):    case hash("C6"):    return (uint8_t) 84;
        case hash("c#6"):   case hash("C#6"):   return (uint8_t) 85;
        case hash("d6"):    case hash("D6"):    return (uint8_t) 86;
        case hash("d#6"):   case hash("D#6"):   return (uint8_t) 87;
        case hash("e6"):    case hash("E6"):    return (uint8_t) 88;
        case hash("f6"):    case hash("F6"):    return (uint8_t) 89;
        case hash("f#6"):   case hash("F#6"):   return (uint8_t) 90;
        case hash("g6"):    case hash("G6"):    return (uint8_t) 91;
        case hash("g#6"):   case hash("G#6"):   return (uint8_t) 92;
        case hash("a6"):    case hash("A6"):    return (uint8_t) 93;
        case hash("a#6"):   case hash("A#6"):   return (uint8_t) 94;
        case hash("b6"):    case hash("B6"):    return (uint8_t) 95;

        case hash("c7"):    case hash("C7"):    return (uint8_t) 96;
        case hash("c#7"):   case hash("C#7"):   return (uint8_t) 97;
        case hash("d7"):    case hash("D7"):    return (uint8_t) 98;
        case hash("d#7"):   case hash("D#7"):   return (uint8_t) 99;
        case hash("e7"):    case hash("E7"):    return (uint8_t) 100;
        case hash("f7"):    case hash("F7"):    return (uint8_t) 101;
        case hash("f#7"):   case hash("F#7"):   return (uint8_t) 102;
        case hash("g7"):    case hash("G7"):    return (uint8_t) 103;
        case hash("g#7"):   case hash("G#7"):   return (uint8_t) 104;
        case hash("a7"):    case hash("A7"):    return (uint8_t) 105;
        case hash("a#7"):   case hash("A#7"):   return (uint8_t) 106;
        case hash("b7"):    case hash("B7"):    return (uint8_t) 107;

        case hash("c8"):    case hash("C8"):    return (uint8_t) 108;
        case hash("c#8"):   case hash("C#8"):   return (uint8_t) 109;
        case hash("d8"):    case hash("D8"):    return (uint8_t) 110;
        case hash("d#8"):   case hash("D#8"):   return (uint8_t) 111;
        case hash("e8"):    case hash("E8"):    return (uint8_t) 112;
        case hash("f8"):    case hash("F8"):    return (uint8_t) 113;
        case hash("f#8"):   case hash("F#8"):   return (uint8_t) 114;
        case hash("g8"):    case hash("G8"):    return (uint8_t) 115;
        case hash("g#8"):   case hash("G#8"):   return (uint8_t) 116;
        case hash("a8"):    case hash("A8"):    return (uint8_t) 117;
        case hash("a#8"):   case hash("A#8"):   return (uint8_t) 118;
        case hash("b8"):    case hash("B8"):    return (uint8_t) 119;

        case hash("c9"):    case hash("C9"):    return (uint8_t) 120;
        case hash("c#9"):   case hash("C#9"):   return (uint8_t) 121;
        case hash("d9"):    case hash("D9"):    return (uint8_t) 122;
        case hash("d#9"):   case hash("D#9"):   return (uint8_t) 123;
        case hash("e9"):    case hash("E9"):    return (uint8_t) 124;
        case hash("f9"):    case hash("F9"):    return (uint8_t) 125;
        case hash("f#9"):   case hash("F#9"):   return (uint8_t) 126;
        case hash("g9"):    case hash("G9"):    return (uint8_t) 127;
        default:                                return {};
    }
}
