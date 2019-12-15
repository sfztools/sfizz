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
#include "ghc/fs_std.hpp"
#include <map>
#include <string>
#include "absl/strings/string_view.h"
#include <vector>

namespace sfz {
class Parser {
public:
    virtual ~Parser() = default;
    virtual bool loadSfzFile(const fs::path& file);
    const std::map<std::string, std::string>& getDefines() const noexcept { return defines; }
    const std::vector<fs::path>& getIncludedFiles() const noexcept { return includedFiles; }
    void disableRecursiveIncludeGuard() { recursiveIncludeGuard = false; }
    void enableRecursiveIncludeGuard() { recursiveIncludeGuard = true; }
protected:
    virtual void callback(absl::string_view header, const std::vector<Opcode>& members) = 0;
    fs::path originalDirectory { fs::current_path() };
private:
    bool recursiveIncludeGuard { false };
    std::map<std::string, std::string> defines;
    std::vector<fs::path> includedFiles;
    std::string aggregatedContent {};
    void readSfzFile(const fs::path& fileName, std::vector<std::string>& lines) noexcept;
};

} // namespace sfz
