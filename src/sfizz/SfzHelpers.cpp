// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfzHelpers.h"
#include "StringViewHelpers.h"

namespace sfz {

bool findHeader(absl::string_view& source, absl::string_view& header, absl::string_view& members)
{
    auto openHeader = source.find("<");
    if (openHeader == absl::string_view::npos)
        return false;

    auto closeHeader = source.find(">", openHeader);
    if (openHeader == absl::string_view::npos)
        return false;

    auto nextHeader = source.find("<", closeHeader);
    header = source.substr(openHeader + 1, closeHeader - openHeader - 1);
    if (nextHeader == absl::string_view::npos) {
        members = trim(source.substr(closeHeader + 1));
        source.remove_prefix(source.length());
    } else {
        members = trim(source.substr(closeHeader + 1, nextHeader - closeHeader - 1));
        source.remove_prefix(nextHeader);
    }

    return true;
}

bool findOpcode(absl::string_view& source, absl::string_view& opcode, absl::string_view& value)
{
    auto opcodeEnd = source.find("=");
    if (opcodeEnd == absl::string_view::npos)
        return false;

    const auto valueStart = opcodeEnd + 1;
    const auto nextOpcodeEnd = source.find("=", valueStart);

    if (nextOpcodeEnd == absl::string_view::npos) {
        opcode = source.substr(0, opcodeEnd);
        value = source.substr(valueStart);
        source.remove_prefix(source.length());
        return true;
    }

    auto valueEnd = nextOpcodeEnd;
    while (source[valueEnd] != ' ' && valueEnd != valueStart)
        valueEnd--;

    opcode = source.substr(0, opcodeEnd);
    value = source.substr(valueStart, valueEnd - valueStart);
    source.remove_prefix(valueEnd);
    return true;
}


bool findDefine(absl::string_view line, absl::string_view& variable, absl::string_view& value)
{
    const auto defPosition = line.find("#define");
    if (defPosition == absl::string_view::npos)
        return false;

    const auto variableStart = line.find("$", 7);
    if (variableStart == absl::string_view::npos)
        return false;

    const auto variableEnd = line.find_first_of(" \r\t\n\f\v", variableStart);
    if (variableEnd == absl::string_view::npos)
        return false;

    const auto valueStart = line.find_first_not_of(" \r\t\n\f\v", variableEnd);
    if (valueStart == absl::string_view::npos)
        return false;

    const auto valueEnd = line.find_first_of(" \r\t\n\f\v", valueStart);
    variable = line.substr(variableStart, variableEnd - variableStart);
    value = valueEnd != absl::string_view::npos
        ? line.substr(valueStart, valueEnd - valueStart)
        : line.substr(valueStart);
    return true;
}

bool findInclude(absl::string_view line, std::string& path)
{
    const auto defPosition = line.find("#include");
    if (defPosition == absl::string_view::npos)
        return false;

    const auto pathStart = line.find("\"", 8);
    if (pathStart == absl::string_view::npos)
        return false;

    const auto pathEnd = line.find("\"", pathStart + 1);
    if (pathEnd == absl::string_view::npos)
        return false;

    path = std::string(line.substr(pathStart + 1, pathEnd - pathStart - 1));
    return true;
}

}
