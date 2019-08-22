#include "Synth.h"
#include <iostream>
#include <absl/flags/parse.h>
#include <absl/types/span.h>

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    auto arguments = absl::ParseCommandLine(argc, argv);
    auto filesToParse = absl::MakeConstSpan(arguments).subspan(1);
    std::cout << "Positional arguments:";
    for (auto& file: filesToParse)
        std::cout << " " << file << ',';
    std::cout << '\n';

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