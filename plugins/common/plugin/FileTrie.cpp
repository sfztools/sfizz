// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileTrie.h"
#include <iostream>
#include <stdexcept>
#include <cassert>

constexpr size_t FileTrie::npos;

fs::path FileTrie::at(size_t index) const
{
    if (index >= entries_.size())
        throw std::out_of_range("FileTrie::at");
    return pathFromEntry(index);
}

fs::path FileTrie::operator[](size_t index) const
{
    assert(index < entries_.size());
    return pathFromEntry(index);
}

fs::path FileTrie::pathFromEntry(size_t index) const
{
    const Entry* currentEntry = &entries_[index];
    fs::path path = fs::u8path(currentEntry->name);

    size_t currentIndex;
    while ((currentIndex = currentEntry->parent) != npos) {
        currentEntry = &entries_[currentIndex];
        path = fs::u8path(currentEntry->name) / path;
    }

    return path;
}

std::ostream& operator<<(std::ostream& os, const FileTrie& trie)
{
    os << '{' << '\n';
    for (size_t i = 0, n = trie.size(); i < n; ++i)
        os << '\t' << i << ':' << ' ' << trie[i] << ',' << '\n';
    os << '}';
    return os;
}

//------------------------------------------------------------------------------
FileTrieBuilder::FileTrieBuilder(size_t initialCapacity)
{
    FileTrie& trie = trie_;
    trie.entries_.reserve(initialCapacity);
}

FileTrie&& FileTrieBuilder::build()
{
    FileTrie& trie = trie_;
    trie.entries_.shrink_to_fit();
    return std::move(trie);
}

size_t FileTrieBuilder::addFile(const fs::path& path)
{
    if (path.empty())
        return FileTrie::npos;

    size_t dirIndex = ensureDirectory(path.parent_path());

    FileTrie& trie = trie_;
    FileTrie::Entry ent;
    ent.parent = dirIndex;
    ent.name = (--path.end())->u8string();

    size_t fileIndex = trie.entries_.size();
    trie.entries_.push_back(std::move(ent));

    return fileIndex;
}

size_t FileTrieBuilder::ensureDirectory(const fs::path& dirPath)
{
    if (dirPath.empty())
        return FileTrie::npos;

    const fs::path::string_type& dirNat = dirPath.native();
    auto it = directories_.find(dirNat);
    if (it != directories_.end())
        return it->second;

    FileTrie& trie = trie_;
    FileTrie::Entry ent;
    ent.parent = FileTrie::npos;
    ent.name = (--dirPath.end())->u8string();
    if (dirPath.has_parent_path()) {
        fs::path parentPath = dirPath.parent_path();
        if (parentPath != dirPath)
            ent.parent = ensureDirectory(parentPath);
    }

    size_t dirIndex = trie.entries_.size();
    trie.entries_.push_back(std::move(ent));

    directories_[dirNat] = dirIndex;

    return dirIndex;
}
