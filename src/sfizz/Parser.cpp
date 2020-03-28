// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Parser.h"
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

bool sfz::OldParser::loadSfzFile(const fs::path& file)
{
    includedFiles.clear();

    const auto sfzFile =
        (file.empty() || file.is_absolute()) ? file : originalDirectory / file;
    if (!fs::exists(sfzFile))
        return false;

    originalDirectory = sfzFile.parent_path();
    includedFiles.push_back(sfzFile);
    std::vector<std::string> lines;
    readSfzFile(sfzFile, lines);

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

void sfz::OldParser::readSfzFile(const fs::path& fileName, std::vector<std::string>& lines) noexcept
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
