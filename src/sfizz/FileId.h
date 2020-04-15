// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <string>
#include <iosfwd>

namespace sfz {

/**
 * @brief Sample file identifier within a file pool.
 */
struct FileId {
    std::string filename;
    bool reverse = false;

    /**
     * @brief Construct a null identifier.
     */
    FileId()
    {
    }

    /**
     * @brief Construct a file identifier, optionally reversed.
     *
     * @param filename
     * @param reverse
     */
    FileId(std::string filename, bool reverse = false)
        : filename(std::move(filename)), reverse(reverse)
    {
    }

    /**
     * @brief Check equality with another identifier.
     *
     * @param other
     */
    bool operator==(const FileId &other) const
    {
        return reverse == other.reverse && filename == other.filename;
    }

    /**
     * @brief Check inequality with another identifier.
     *
     * @param other
     */
    bool operator!=(const FileId &other) const
    {
        return !operator==(other);
    }
};

}

namespace std {
    template <> struct hash<sfz::FileId> {
        size_t operator()(const sfz::FileId &id) const;
    };
}

std::ostream &operator<<(std::ostream &os, const sfz::FileId &fileId);
