#pragma once
#include "Opcode.h"
#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace sfz {
namespace Regexes {
    inline static std::regex includes { R"V(#include\s*"(.*?)".*$)V", std::regex::optimize };
    inline static std::regex defines { R"(#define\s*(\$[a-zA-Z0-9]+)\s+([a-zA-Z0-9]+)(?=\s|$))", std::regex::optimize };
    inline static std::regex headers { R"(<(.*?)>(.*?)(?=<|$))", std::regex::optimize };
    inline static std::regex members { R"(([a-zA-Z0-9_]+)=([a-zA-Z0-9-_#.\/\s\\\(\),\*]+)(?![a-zA-Z0-9_]*=))", std::regex::optimize };
    inline static std::regex opcodeParameters { R"(([a-zA-Z0-9_]+?)([0-9]+)$)", std::regex::optimize };
}

class Parser {
public:
    virtual bool loadSfzFile(const std::filesystem::path& file);
    const std::map<std::string, std::string>& getDefines() const noexcept { return defines; }
    const std::vector<std::filesystem::path>& getIncludedFiles() const noexcept { return includedFiles; }
    void disableRecursiveIncludeGuard() { recursiveIncludeGuard = false; }
    void enableRecursiveIncludeGuard() { recursiveIncludeGuard = true; }

protected:
    virtual void callback(std::string_view header, const std::vector<Opcode>& members) = 0;
    std::filesystem::path rootDirectory { std::filesystem::current_path() };

private:
    bool recursiveIncludeGuard { false };
    std::map<std::string, std::string> defines;
    std::vector<std::filesystem::path> includedFiles;
    std::string aggregatedContent {};
    void readSfzFile(const std::filesystem::path& fileName, std::vector<std::string>& lines) noexcept;
};

} // namespace sfz