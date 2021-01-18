// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "FileTrie.h"
#include <absl/types/span.h>
#include <absl/types/optional.h>
#include <ghc/fs_std.hpp>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>

class SfzFileScan {
public:
    static SfzFileScan& getInstance();
    bool locateRealFile(const fs::path& pathOrig, fs::path& pathFound);

private:
    typedef std::chrono::steady_clock clock;
    std::mutex mutex;
    absl::optional<clock::time_point> completion_time_;
    FileTrie file_trie_;
    std::unordered_map<std::string, std::list<size_t>> file_index_;

    bool isExpired() const;
    void refreshScan(bool force = false);
    static std::string keyOf(const fs::path& path);
    static bool pathIsSfz(const fs::path& path);
    static const fs::path& electBestMatch(const fs::path& path, absl::Span<const fs::path> candidates);
};

namespace SfizzPaths {
std::vector<fs::path> getSfzSearchPaths();
absl::optional<fs::path> getSfzConfigDefaultPath();
void setSfzConfigDefaultPath(const fs::path& path);
fs::path getSfzFallbackDefaultPath();
std::vector<fs::path> getEnvironmentSfzPaths();
} // namespace SfizzPaths
