// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Parser.h"
#include "Config.h"
#include "StringViewHelpers.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_cat.h"
#include <algorithm>
#include <fstream>

void removeCommentOnLine(absl::string_view& line)
{
    auto position = line.find("//");
    if (position != line.npos)
        line.remove_suffix(line.size() - position);
}

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

bool sfz::Parser::loadSfzFile(const fs::path& file)
{
    const auto sfzFile = file.is_absolute() ? file : originalDirectory / file;
    if (!fs::exists(sfzFile))
        return false;

    originalDirectory = file.parent_path();
    std::vector<std::string> lines;
    readSfzFile(file, lines);

    aggregatedContent = absl::StrJoin(lines, " ");
    absl::string_view aggregatedView { aggregatedContent };
    absl::string_view header;
    absl::string_view members;

    std::vector<Opcode> currentMembers;
    while (findHeader(aggregatedView, header, members)) {

        absl::string_view opcode;
        absl::string_view value;

        // Store or handle members
        while(findOpcode(members, opcode, value))
            currentMembers.emplace_back(opcode, value);
        
        callback(header, currentMembers);
        currentMembers.clear();
    }

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

void sfz::Parser::readSfzFile(const fs::path& fileName, std::vector<std::string>& lines) noexcept
{
    std::ifstream fileStream(fileName.c_str());
    if (!fileStream)
        return;

    std::string tmpString;
    std::string includePath;
    absl::string_view variable;
    absl::string_view value;
    while (std::getline(fileStream, tmpString)) {
        absl::string_view tmpView { tmpString };

        removeCommentOnLine(tmpView);
        trimInPlace(tmpView);

        if (tmpView.empty())
            continue;

        // New #include
        if (findInclude(tmpView, includePath)) {
            std::replace(includePath.begin(), includePath.end(), '\\', '/');
            const auto newFile = originalDirectory / includePath;
            auto alreadyIncluded = std::find(includedFiles.begin(), includedFiles.end(), newFile);
            if (fs::exists(newFile)) {
                if (alreadyIncluded == includedFiles.end()) {
                    includedFiles.push_back(newFile);
                    readSfzFile(newFile, lines);
                } else if (!recursiveIncludeGuard) {
                    readSfzFile(newFile, lines);
                }
            }
            continue;
        }

        // New #define
        if (findDefine(tmpView, variable, value)) {
            
            defines[std::string(variable)] = std::string(value);
            continue;
        }

        // Replace defined variables starting with $
        std::string newString;
        newString.reserve(tmpView.length());
        std::string::size_type lastPos = 0;
        std::string::size_type findPos = tmpView.find(sfz::config::defineCharacter, lastPos);

        while (findPos < tmpView.npos) {
            absl::StrAppend(&newString, tmpView.substr(lastPos, findPos - lastPos));

            const auto defineEnd = tmpView.find_first_of("= \r\t\n\f\v", findPos);
            const auto candidate = tmpView.substr(findPos, defineEnd - findPos);
            for (auto& definePair : defines) {
                if (candidate == definePair.first) {
                    absl::StrAppend(&newString, definePair.second);
                    lastPos = findPos + definePair.first.length();
                    break;
                }
            }


            if (lastPos <= findPos) {
                newString += sfz::config::defineCharacter;
                lastPos = findPos + 1;
            }

            findPos = tmpView.find(sfz::config::defineCharacter, lastPos);
        }

        // Copy the rest of the string
        absl::StrAppend(&newString, tmpView.substr(lastPos));
        lines.push_back(std::move(newString));
    }
}
