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

#include "parser/Parser.h"
#include "cxxopts.hpp"
#include <iostream>

class PrintingParser: public sfz::Parser::Listener {
public:
    PrintingParser()
    {
        parser.setListener(this);
    }

    void parseFile(const fs::path& path)
    {
        parser.parseFile(path);
    }

    void showHierarchy(bool show)
    {
        _showHierarchy = show;
    }
protected:
    void onParseFullBlock(const std::string& header, const std::vector<sfz::Opcode>& members) override
    {
        auto printBlock = [&]() {
            std::cout << "<" << header << ">" << ' ';
            printMembers(members);
            std::cout << '\n';
        };
        switch (hash(header))
        {
        case hash("global"):
            globalMembers = members;
            masterMembers.clear();
            groupMembers.clear();
            if (_showHierarchy)
                printBlock();
            break;
        case hash("master"):
            masterMembers = members;
            groupMembers.clear();
            if (_showHierarchy)
                printBlock();
            break;
        case hash("group"):
            groupMembers = members;
            if (_showHierarchy)
                printBlock();
            break;
        case hash("region"):
            std::cout << "<" << header << ">" << ' ';
            if (!_showHierarchy) {
                printMembers(globalMembers);
                printMembers(masterMembers);
                printMembers(groupMembers);
            }
            printMembers(members);
            std::cout << '\n';
            break;
        default:
            printBlock();
            break;
        }
    }

    /**
     * @brief The parser callback when an error occurs.
     */
    void onParseError(const sfz::SourceRange& range, const std::string& message) override
    {
        std::cerr <<  "\033[1;31m" <<  range.start.filePath->string() << ":" << range.start.lineNumber << "\t" << message <<"\033[0m" << '\n';
    }

    /**
     * @brief The parser callback when a warning occurs.
     */
    void onParseWarning(const sfz::SourceRange& range, const std::string& message) override
    {
        std::cout <<  "\033[1;33m" <<  range.start.filePath->string() << ":" << range.start.lineNumber << "\t" << message <<"\033[0m" << '\n';
    }

private:
    std::vector<sfz::Opcode> globalMembers;
    std::vector<sfz::Opcode> masterMembers;
    std::vector<sfz::Opcode> groupMembers;
    void printMembers(const std::vector<sfz::Opcode>& members)
    {
        for (auto& member: members)
        {
            std::cout << member.opcode;
            std::cout << "=" << member.value;
            std::cout << ' ';
        }
    }
    sfz::Parser parser;
    bool _showHierarchy;
};

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    cxxopts::Options options("sfizz-flat", "Flattens an sfz file with all defines and includes");
    bool no_hierarchy { false };
    options.add_options()
        ("no-hierarchy", "Push all the opcodes in the regions", cxxopts::value(no_hierarchy))
    ;
    options.add_options("hidden")
        ("help", "Show help", cxxopts::value<bool>())
        ("file", "Root SFZ file", cxxopts::value<std::string>())
    ;
    options.parse_positional({"file"});
    auto params = [&]() {
        try { return options.parse(argc, argv); }
        catch (std::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << '\n';
            std::exit(-1);
        }
    }();

    if (!params.count("file")) {
        std::cerr << "No file given" << '\n';
        std::cout << options.help({""}) << '\n';
        std::exit(-1);
    }

    if (params.count("help"))
    {
      std::cout << options.help({""}) << '\n';
      exit(0);
    }

    PrintingParser parser;
    parser.showHierarchy(!no_hierarchy);
    parser.parseFile(params["file"].as<std::string>());
    return 0;
}
