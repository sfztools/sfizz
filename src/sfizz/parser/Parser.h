// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ghc/fs_std.hpp"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include <string>
#include <memory>

namespace sfz {

class Reader;
struct SourceLocation;
struct SourceRange;

/**
 * @brief Context-dependent parser for SFZ files
 */
class Parser {
public:
    Parser();
    ~Parser();

    void addDefinition(absl::string_view id, absl::string_view value);
    void parseFile(const fs::path& path);

    size_t getErrorCount() const noexcept { return _errorCount; }
    size_t getWarningCount() const noexcept { return _warningCount; }

    class Listener {
    public:
        virtual void onParseBegin() = 0;
        virtual void onParseEnd() = 0;
        virtual void onParseHeader(const SourceRange& range, const std::string& header) = 0;
        virtual void onParseOpcode(const SourceRange& rangeOpcode, const SourceRange& rangeValue, const std::string& name, const std::string& value) = 0;
        virtual void onParseError(const SourceRange& range, const std::string& message) = 0;
        virtual void onParseWarning(const SourceRange& range, const std::string& message) = 0;
    };

    void setListener(Listener* listener) noexcept { _listener = listener; }

private:
    void includeNewFile(const fs::path& path);
    void processTopLevel();
    void processDirective();
    void processHeader();
    void processOpcode();

    // errors and warnings
    void emitError(const SourceRange& range, const std::string& message);
    void emitWarning(const SourceRange& range, const std::string& message);

    // recover after error
    void recover();

    // helpers
    static bool hasComment(Reader& reader);
    static size_t skipComment(Reader& reader);
    static void trimRight(std::string& text);
    static size_t extractToEol(Reader& reader, std::string* dst); // ignores comment
    std::string expandDollarVars(const SourceRange& range, absl::string_view src);

    // predicates
    static bool isIdentifierChar(char c);
    static bool isSpaceChar(char c);
    static bool isIdentifier(absl::string_view s);

private:
    Listener* _listener = nullptr;

    fs::path _originalDirectory { fs::current_path() };
    absl::flat_hash_map<std::string, std::string> _definitions;

    // a current list of files included, last one at the back
    std::vector<std::unique_ptr<Reader>> _included;

    // recursive include guard
    absl::flat_hash_set<std::string> _pathsIncluded;

    // parsing state
    std::string _lastHeader;

    // errors and warnings
    size_t _errorCount = 0;
    size_t _warningCount = 0;
};

/**
 * @brief Source file location for errors and warnings.
 */
struct SourceLocation {
    std::shared_ptr<fs::path> filePath;
    size_t lineNumber = 0;
    size_t columnNumber = 0;
};

/**
 * @brief Range of source file.
 */
struct SourceRange {
    SourceLocation start;
    SourceLocation end;
};

} // namespace sfz
