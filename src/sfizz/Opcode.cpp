// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Opcode.h"
#include "StringViewHelpers.h"

sfz::Opcode::Opcode(absl::string_view inputOpcode, absl::string_view inputValue)
    : opcode(inputOpcode)
    , value(inputValue)
{
    const auto lastCharIndex = inputOpcode.find_last_not_of("1234567890");
    if (lastCharIndex != inputOpcode.npos) {
        int returnedValue;
        absl::string_view parameterView = inputOpcode;
        parameterView.remove_prefix(lastCharIndex + 1);
        if (absl::SimpleAtoi(parameterView, &returnedValue)) {
            parameter = returnedValue;
            opcode.remove_suffix(opcode.size() - lastCharIndex - 1);
        }
    }
    trimInPlace(value);
    trimInPlace(opcode);
}
