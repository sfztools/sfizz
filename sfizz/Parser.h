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

#pragma once
#include "Opcode.h"
#include "filesystem.h"
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
