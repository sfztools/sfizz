// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Parser.h"
#include "ghc/fs_std.hpp"
#include "absl/strings/string_view.h"
#include <string>

namespace sfz {

/**
 * @brief Utility to extract characters and strings from a source of any kind.
 */
class Reader {
public:
    explicit Reader(const fs::path& filePath);
    virtual ~Reader() {}

    /**
     * @brief Get the current source location.
     */
    const SourceLocation& location() const { return _loc; }

    /**
     * @brief Value of the end-of-file marker.
     */
    static constexpr int kEof = std::char_traits<char>::eof();

    /**
     * @brief Extract the next character.
     */
    int getChar();

    /**
     * @brief Get the next character without extracting it.
     */
    int peekChar();

    /**
     * @brief Put a previously extracted character back into the reader.
     */
    void putBackChar(int c);

    /**
     * @brief Put some previously extracted characters back into the reader.
     */
    void putBackChars(absl::string_view characters);

    /**
     * @brief Extract as long as a predicate holds on the next character.
     */
    template <class P> size_t extractWhile(std::string* dst, const P& pred);

    /**
     * @brief Extract until as a predicate does not hold on the next character.
     */
    template <class P> size_t extractUntil(std::string* dst, const P& pred);

    /**
     * @brief Extract a character if it is equal to the expected value.
     */
    bool extractExactChar(char c);

    /**
     * @brief Skip characters which belong to a given set
     */
    size_t skipChars(absl::string_view chars);

    /**
     * @brief Skip as long as a predicate holds on the next character.
     */
    template <class P> size_t skipWhile(const P& pred);

    /**
     * @brief Skip until as a predicate does not hold on the next character.
     */
    template <class P> size_t skipUntil(const P& pred);

    /**
     * @brief Check if the reader has no more characters.
     */
    bool hasEof();

    /**
     * @brief Check if the reader has one of the following characters next.
     */
    bool hasOneOfChars(absl::string_view chars);

protected:
    virtual int getNextStreamByte() = 0;

private:
    void updateSourceLocationAdding(int byte);
    void updateSourceLocationRemoving(int byte);

private:
    std::string _accum; // new characters at the front, old at the back
    SourceLocation _loc;
    std::vector<int> _lineNumColumns;
};

/**
 * @brief File-based version of Reader.
 */
class FileReader : public Reader {
public:
    explicit FileReader(const fs::path& filePath);
    bool hasError();

protected:
    int getNextStreamByte() override;

private:
    fs::ifstream _fileStream;
};

}  // namespace sfz

#include "ParserPrivate.hpp"
