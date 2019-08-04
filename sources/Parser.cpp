#include "Parser.h"
#include "Helpers.h"
#include "Globals.h"
#include "absl/strings/str_join.h"
#include <fstream>
#include <algorithm>

using svregex_iterator = std::regex_iterator<std::string_view::const_iterator>;
using svmatch_results = std::match_results<std::string_view::const_iterator>;

void removeCommentOnLine(std::string_view& line)
{
	if (auto position = line.find("//"); position != line.npos)
		line.remove_suffix(line.size() - position);
}

bool sfz::Parser::loadSfzFile(const std::filesystem::path& file)
{
    const auto sfzFile = file.is_absolute() ? file : rootDirectory / file;
	if (!std::filesystem::exists(sfzFile))
		return false;

    rootDirectory = file.parent_path();
	std::vector<std::string> lines;
	readSfzFile(file, lines);

    aggregatedContent = absl::StrJoin(lines, " ");
	const std::string_view aggregatedView { aggregatedContent };

    svregex_iterator headerIterator(aggregatedView.cbegin(), aggregatedView.cend(), sfz::Regexes::headers);
	const auto regexEnd = svregex_iterator();

    std::vector<Opcode> currentMembers;

    for (; headerIterator != regexEnd; ++headerIterator)
  	{
		svmatch_results headerMatch = *headerIterator;

		// Can't use uniform initialization here because it generates narrowing conversions
		const std::string_view header(&*headerMatch[1].first, headerMatch[1].length());
		const std::string_view members(&*headerMatch[2].first, headerMatch[2].length());
		auto paramIterator = svregex_iterator (members.cbegin(), members.cend(), sfz::Regexes::members);

		// Store or handle members
		for (; paramIterator != regexEnd; ++paramIterator)
		{
			const svmatch_results paramMatch = *paramIterator;
			const std::string_view opcode(&*paramMatch[1].first, paramMatch[1].length());
			const std::string_view value(&*paramMatch[2].first, paramMatch[2].length());
            currentMembers.emplace_back(opcode, value);		
		}
		callback(header, currentMembers);
        currentMembers.clear();
	}

    return true;
}

void sfz::Parser::readSfzFile(const std::filesystem::path& fileName, std::vector<std::string>& lines) noexcept
{
	std::ifstream fileStream(fileName.c_str());
	if (!fileStream)
		return;

	// spdlog::info("Including file {}", fileName.string());
	svmatch_results includeMatch;
	svmatch_results defineMatch;

	std::string tmpString;
	while (std::getline(fileStream, tmpString))
	{
		std::string_view tmpView { tmpString };

		removeCommentOnLine(tmpView);
		trimInPlace(tmpView);

		if (tmpView.empty())
			continue;

		// New #include
		if (std::regex_search(tmpView.begin(), tmpView.end(), includeMatch, sfz::Regexes::includes))
		{
			auto includePath = includeMatch.str(1);
			std::replace(includePath.begin(), includePath.end(), '\\', '/');
			const auto newFile = rootDirectory / includePath;			
			auto alreadyIncluded = std::find(includedFiles.begin(), includedFiles.end(), newFile);
			if (std::filesystem::exists(newFile))
			{
				if (alreadyIncluded == includedFiles.end())
				{
					includedFiles.push_back(newFile);
					readSfzFile(newFile, lines);
				}
				else if (!recursiveIncludeGuard)
				{
					readSfzFile(newFile, lines);
				}
			}
			continue;
		}

		// New #define
		if (std::regex_search(tmpView.begin(), tmpView.end(), defineMatch, sfz::Regexes::defines))
		{
			defines[defineMatch.str(1)] = defineMatch.str(2);
			continue;
		}

		// Replace defined variables starting with $
		std::string newString;
		newString.reserve(tmpView.length());
		std::string::size_type lastPos = 0;
    	std::string::size_type findPos = tmpView.find(sfz::config::defineCharacter, lastPos);

		while(findPos < tmpView.npos)
		{
			newString.append(tmpView, lastPos, findPos - lastPos);

			for (auto& definePair: defines)
			{
				std::string_view candidate = tmpView.substr(findPos, definePair.first.length());
				if (candidate == definePair.first)
				{
					newString += definePair.second;
					lastPos = findPos + definePair.first.length();
					break;
				}
			}
			
			if (lastPos <= findPos)
			{
				newString += sfz::config::defineCharacter;
				lastPos = findPos + 1;
			}

			findPos = tmpView.find(sfz::config::defineCharacter, lastPos);
		}

		// Copy the rest of the string
		newString += tmpView.substr(lastPos);
		lines.push_back(std::move(newString));		
	}
}