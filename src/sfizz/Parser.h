// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Opcode.h"
#include "ghc/fs_std.hpp"
#include <map>
#include <string>
#include "absl/strings/string_view.h"
#include <vector>

namespace sfz {
class OldParser {
public:
    virtual ~OldParser() = default;
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
