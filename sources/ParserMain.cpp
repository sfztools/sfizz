#include "Parser.h"
#include <iostream>
#include <filesystem>
#include <string_view>
#include <absl/flags/parse.h>
#include <absl/types/span.h>

class PrintingParser: public sfz::Parser
{
public:
    int getNumRegions() const noexcept { return numRegions; }
    int getNumGroups() const noexcept { return numGroups; }
    int getNumMasters() const noexcept { return numMasters; }
    int getNumCurves() const noexcept { return numCurves; }
protected:
    void callback(std::string_view header, const std::vector<sfz::Opcode>& members [[maybe_unused]]) final
    {
        switch (hash(header))
        {
        case hash("master"): numMasters++; break;
        case hash("group"): numGroups++; break;
        case hash("region"): numRegions++; break;
        case hash("curve"): numCurves++; break;
        }
        // std::cout << "[" << header << "]" << '\t';
        // for (auto& member: members)
        // {
        //     std::cout << member.opcode << "=" << member.value;
        //     if (member.parameter)
        //         std::cout << " (" << (int)*member.parameter << ")";
        //     std::cout << ' ';

        // }
        // std::cout << '\n';
    }
private:
    int numRegions { 0 };
    int numGroups { 0 };
    int numMasters { 0 };
    int numCurves { 0 };
};

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    auto arguments = absl::ParseCommandLine(argc, argv);
    auto filesToParse = absl::MakeConstSpan(arguments).subspan(1);
    std::cout << "Positional arguments:";
    for (auto& file: filesToParse)
        std::cout << " " << file << ',';
    std::cout << '\n';
    
    PrintingParser parser;
    std::filesystem::path filename { filesToParse[0] };
    parser.loadSfzFile(filename);  
    std::cout << "==========" << '\n';
    std::cout << "Total:" << '\n';
    std::cout << "\tMasters: " << parser.getNumMasters() << '\n';
    std::cout << "\tGroups: " << parser.getNumGroups() << '\n';
    std::cout << "\tRegions: " << parser.getNumRegions() << '\n';
    std::cout << "\tCurves: " << parser.getNumCurves() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Included files:" << '\n';
    for (auto& file: parser.getIncludedFiles())
        std::cout << '\t' << file.c_str() << '\n';
    std::cout << "==========" << '\n';
    std::cout << "Defines:" << '\n';
    for (auto& define: parser.getDefines())
        std::cout << '\t' << define.first << '=' << define.second << '\n';
    // spdlog::info("Done!");
    return 0;
}