#include "Opcode.h"

sfz::Opcode::Opcode(std::string_view inputOpcode, std::string_view inputValue)
    : opcode(inputOpcode)
    , value(inputValue)
{
    if (const auto lastCharIndex = inputOpcode.find_last_not_of("1234567890"); lastCharIndex != inputOpcode.npos) {
        int returnedValue;
        std::string_view parameterView = inputOpcode;
        parameterView.remove_prefix(lastCharIndex + 1);
        if (absl::SimpleAtoi(parameterView, &returnedValue)) {
            parameter = returnedValue;
            opcode.remove_suffix(opcode.size() - lastCharIndex - 1);
        }
    }
    trimInPlace(value);
    trimInPlace(opcode);
}