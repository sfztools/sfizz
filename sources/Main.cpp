#include "Synth.h"
#include <iostream>
#include "cxxopts.hpp"

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::vector<std::string> filesToParse;
    cxxopts::Options options("sfzparser", "Parses an sfz file and prints the output");
    options.add_options()("positional", "SFZ files to parse", cxxopts::value<std::vector<std::string>>(filesToParse));
    options.parse_positional({"positional"});
    auto result = options.parse(argc, argv);
    if (filesToParse.size() == 0)
    {
        std::cout << options.help() << '\n';
        return -1;
    }

    sfz::Synth synth;
    std::filesystem::path filename { filesToParse[0] };
    synth.loadSfzFile(filename);  
    std::cout << "==========" << '\n';
    std::cout << "Total:" << '\n';
    std::cout << "\tMasters: " << synth.getNumMasters() << '\n';
    std::cout << "\tGroups: " << synth.getNumGroups() << '\n';
    std::cout << "\tRegions: " << synth.getNumRegions() << '\n';
    std::cout << "\tCurves: " << synth.getNumCurves() << '\n';
    std::cout << "\tPreloadedSamples: " << synth.getNumPreloadedSamples() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Included files:" << '\n';
    for (auto& file: synth.getIncludedFiles())
        std::cout << '\t' << file.string() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Defines:" << '\n';
    for (auto& define: synth.getDefines())
        std::cout << '\t' << define.first << '=' << define.second << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Unknown opcodes:";
    for (auto& opcode: synth.getUnknownOpcodes())
        std::cout << opcode << ',';
    std::cout << '\n';
    return 0;
}