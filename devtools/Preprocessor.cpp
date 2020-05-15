// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
  This program reads a SFZ file, and outputs it back into a single file
  with all the includes and definitions processed.

  It can serve to facilitate identifying problems, whether these are related to
  the parser or complicated instrument structures.
 */

#include "parser/Parser.h"
#include <iostream>

class MyParserListener : public sfz::Parser::Listener {
public:
    explicit MyParserListener(sfz::Parser& parser)
        : _parser(parser)
    {
    }

protected:
    void onParseFullBlock(const std::string& header, const std::vector<sfz::Opcode>& opcodes) override
    {
        std::cout << '\n';
        std::cout << '<' << header << '>' << '\n';
        for (const sfz::Opcode& opc : opcodes)
            std::cout << opc.opcode << '=' << opc.value << '\n';
    }

    void onParseError(const sfz::SourceRange& range, const std::string& message) override
    {
        const auto relativePath = range.start.filePath->lexically_relative(_parser.originalDirectory());
        std::cerr << "Parse error in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
    }

    void onParseWarning(const sfz::SourceRange& range, const std::string& message) override
    {
        const auto relativePath = range.start.filePath->lexically_relative(_parser.originalDirectory());
        std::cerr << "Parse warning in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
    }

private:
    sfz::Parser& _parser;
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Please indicate the SFZ file path.\n";
        return 1;
    }

    const fs::path sfzFilePath { argv[1] };

    sfz::Parser parser;
    MyParserListener listener(parser);

    parser.setListener(&listener);
    parser.parseFile(argv[1]);

    if (parser.getErrorCount() > 0)
        return 1;

    return 0;
}
