// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <ghc/fs_std.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <iosfwd>
#include <cstddef>

class FileTrie {
public:
    static constexpr size_t npos = ~size_t(0);

    size_t size() const noexcept { return entries_.size(); }
    fs::path at(size_t index) const;
    fs::path operator[](size_t index) const;
    void clear() noexcept { entries_.clear(); }

private:
    fs::path pathFromEntry(size_t index) const;

private:
    struct Entry {
        size_t parent = npos;
        std::string name;
    };
    std::vector<Entry> entries_;

    friend class FileTrieBuilder;
};

std::ostream& operator<<(std::ostream& os, const FileTrie& trie);

//------------------------------------------------------------------------------
class FileTrieBuilder {
public:
    explicit FileTrieBuilder(size_t initialCapacity = 8192);
    FileTrie&& build();
    size_t addFile(const fs::path& path);

private:
    size_t ensureDirectory(const fs::path& dirPath);

private:
    FileTrie trie_;
    std::unordered_map<fs::path::string_type, size_t> directories_;
};
