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
#include "SfzHelpers.h"
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

bool sfz::Parser::loadSfzFile(const fs::path& file)
{
    includedFiles.clear();

    const auto sfzFile = file.is_absolute() ? file : originalDirectory / file;
    if (!fs::exists(sfzFile))
        return false;

    originalDirectory = file.parent_path();
    includedFiles.push_back(file);
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
