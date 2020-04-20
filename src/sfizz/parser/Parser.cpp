// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Parser.h"
#include "ParserPrivate.h"
#include "absl/memory/memory.h"
#include <cassert>

namespace sfz {

Parser::Parser()
{
}

Parser::~Parser()
{
}

void Parser::reset()
{
    _pathsIncluded.clear();
    _currentDefinitions = _externalDefinitions;
    _currentHeader.reset();
    _currentOpcodes.clear();
    _errorCount = 0;
    _warningCount = 0;
}

void Parser::addExternalDefinition(absl::string_view id, absl::string_view value)
{
    _externalDefinitions[id] = std::string(value);
}

void Parser::clearExternalDefinitions()
{
    _externalDefinitions.clear();
}

void Parser::parseFile(const fs::path& path)
{
    parseVirtualFile(path, nullptr);
}

void Parser::parseString(const fs::path& path, absl::string_view sfzView)
{
    parseVirtualFile(path, absl::make_unique<StringViewReader>(path, sfzView));
}

void Parser::parseVirtualFile(const fs::path& path, std::unique_ptr<Reader> reader)
{
    reset();

    if (_listener)
        _listener->onParseBegin();

    includeNewFile(path, std::move(reader), {});
    processTopLevel();
    flushCurrentHeader();

    if (_listener)
        _listener->onParseEnd();
}

void Parser::includeNewFile(const fs::path& path, std::unique_ptr<Reader> reader, const SourceRange& includeStmtRange)
{
    fs::path fullPath =
        (path.empty() || path.is_absolute()) ? path : _originalDirectory / path;

    if (_pathsIncluded.empty())
        _originalDirectory = fullPath.parent_path();
    else if (_pathsIncluded.find(fullPath.string()) != _pathsIncluded.end()) {
        if (_recursiveIncludeGuardEnabled)
            return;
    }

    auto makeErrorRange = [&]() -> SourceRange {
        if (!includeStmtRange) {
            SourceLocation loc;
            loc.filePath = std::make_shared<fs::path>(fullPath);
            return {loc, loc};
        }
        return includeStmtRange;
    };

    if (_included.size() == _maxIncludeDepth) {
        emitError(makeErrorRange(), "Exceeded maximum include depth (" + std::to_string(_maxIncludeDepth) + ")");
        return;
    }

    if (!reader) {
        auto fileReader = absl::make_unique<FileReader>(fullPath);
        if (fileReader->hasError()) {
            SourceLocation loc = fileReader->location();
            emitError(makeErrorRange(), "Cannot open file for reading: " + fullPath.string());
            return;
        }
        reader = std::move(fileReader);
    }

    _pathsIncluded.insert(fullPath.string());
    _included.push_back(std::move(reader));
}

void Parser::addDefinition(absl::string_view id, absl::string_view value)
{
    _currentDefinitions[id] = std::string(value);
}

void Parser::processTopLevel()
{
    while (!_included.empty()) {
        Reader& reader = *_included.back();

        while (reader.skipChars(" \t\r\n") || skipComment());

        switch (reader.peekChar()) {
        case Reader::kEof:
            _included.pop_back();
            break;
        case '#':
            processDirective();
            break;
        case '<':
            processHeader();
            break;
        default:
            processOpcode();
            break;
        }
    }
}

void Parser::processDirective()
{
    Reader& reader = *_included.back();
    SourceLocation start = reader.location();

    if (reader.getChar() != '#') {
        SourceLocation end = reader.location();
        emitError({ start, end }, "Expected `#` at start of directive.");
        recover();
        return;
    }

    std::string directive;
    reader.extractWhile(&directive, isIdentifierChar);

    if (directive == "define") {
        reader.skipChars(" \t");

        std::string id;
        if (!reader.extractExactChar('$') || !reader.extractWhile(&id, isIdentifierChar)) {
            SourceLocation end = reader.location();
            emitError({ start, end }, "Expected $identifier after #define.");
            recover();
            return;
        }

        reader.skipChars(" \t");

        std::string value;
        extractToEol(reader, &value);
        trimRight(value);

        addDefinition(id, value);
    }
    else if (directive == "include") {
        reader.skipChars(" \t");

        std::string path;
        bool valid = false;

        if (reader.extractExactChar('"')) {
            reader.extractWhile(&path, [](char c) { return c != '"' && c != '\r' && c != '\n'; });
            valid = reader.extractExactChar('"');
        }

        SourceLocation end = reader.location();

        if (!valid) {
            emitError({ start, end }, "Expected \"file.sfz\" after #include.");
            recover();
            return;
        }

        std::replace(path.begin(), path.end(), '\\', '/');
        includeNewFile(path, nullptr, { start, end });
    }
    else {
        SourceLocation end = reader.location();
        emitError({ start, end }, "Unrecognized directive `" + directive + "`");
        recover();
    }
}

void Parser::processHeader()
{
    Reader& reader = *_included.back();
    SourceLocation start = reader.location();

    if (reader.getChar() != '<') {
        SourceLocation end = reader.location();
        emitError({ start, end }, "Expected `<` at start of header.");
        recover();
        return;
    }

    std::string name;
    reader.extractWhile(&name, [](char c) {
        return c != '\r' && c != '\n' && c != '>';
    });

    if (reader.peekChar() != '>') {
        SourceLocation end = reader.location();
        emitError({ start, end }, "Expected `>` at end of header.");
        recover();
        return;
    }
    reader.getChar();
    SourceLocation end = reader.location();

    if (!isIdentifier(name)) {
        emitError({ start, end }, "The header name `" + name + "` is not a valid identifier.");
        recover();
        return;
    }

    flushCurrentHeader();

    _currentHeader = name;
    if (_listener)
        _listener->onParseHeader({ start, end }, name);
}

void Parser::processOpcode()
{
    Reader& reader = *_included.back();
    SourceLocation opcodeStart = reader.location();

    auto isRawOpcodeNameChar = [](char c) {
        return isIdentifierChar(c) || c == '$';
    };

    std::string nameRaw;
    reader.extractWhile(&nameRaw, isRawOpcodeNameChar);

    SourceLocation opcodeEnd = reader.location();

    if (nameRaw.empty()) {
        emitError({ opcodeStart, opcodeEnd }, "Expected opcode name.");
        recover();
        return;
    }

    if (reader.peekChar() != '=') {
        emitError({ opcodeStart, opcodeEnd }, "Expected `=` after opcode name.");
        recover();
        return;
    }

    std::string nameExpanded = expandDollarVars({ opcodeStart, opcodeEnd }, nameRaw);
    if (!isIdentifier(nameExpanded)) {
        emitError({ opcodeStart, opcodeEnd }, "The opcode name `" + nameExpanded + "` is not a valid identifier.");
        recover();
        return;
    }

    reader.getChar();

    SourceLocation valueStart = reader.location();
    std::string valueRaw;
    extractToEol(reader, &valueRaw);

    // if a "=" or "<" character was hit, it means we read too far
    size_t position = valueRaw.find_first_of("=<");
    if (position != valueRaw.npos) {
        char hitChar = valueRaw[position];

        // if it was "=", rewind before the opcode name and spaces preceding
        if (hitChar == '=') {
            while (position > 0 && isRawOpcodeNameChar(valueRaw[position - 1]))
                --position;
            while (position > 0 && isSpaceChar(valueRaw[position - 1]))
                --position;
        }

        absl::string_view excess(&valueRaw[position], valueRaw.size() - position);
        reader.putBackChars(excess);
        valueRaw.resize(position);

        // ensure that we are landing back next to a space char
        if (hitChar == '=' && !reader.hasOneOfChars(" \t\r\n")) {
            SourceLocation end = reader.location();
            emitError({ valueStart, end }, "Unexpected `=` in opcode value.");
            recover();
            return;
        }
    }

    while (!valueRaw.empty() && isSpaceChar(valueRaw.back())) {
        reader.putBackChar(valueRaw.back());
        valueRaw.pop_back();
    }
    SourceLocation valueEnd = reader.location();

    if (!_currentHeader)
        emitWarning({ opcodeStart, valueEnd }, "The opcode is not under any header.");

    std::string valueExpanded = expandDollarVars({ valueStart, valueEnd }, valueRaw);
    _currentOpcodes.emplace_back(nameExpanded, valueExpanded);

    if (_listener)
        _listener->onParseOpcode({ opcodeStart, opcodeEnd }, { valueStart, valueEnd }, nameExpanded, valueExpanded);
}

void Parser::emitError(const SourceRange& range, const std::string& message)
{
    ++_errorCount;
    if (_listener)
        _listener->onParseError(range, message);
}

void Parser::emitWarning(const SourceRange& range, const std::string& message)
{
    ++_warningCount;
    if (_listener)
        _listener->onParseWarning(range, message);
}

void Parser::recover()
{
    Reader& reader = *_included.back();

    // skip the current line and let the parser proceed at the next
    reader.skipWhile([](char c) { return c != '\n'; });
}

void Parser::flushCurrentHeader()
{
    if (_currentHeader) {
        if (_listener)
            _listener->onParseFullBlock(*_currentHeader, _currentOpcodes);
        _currentHeader.reset();
    }

    _currentOpcodes.clear();
}

Parser::CommentType Parser::getCommentType(Reader& reader)
{
    if (reader.peekChar() != '/')
        return CommentType::None;

    reader.getChar();

    CommentType ret = CommentType::None;

    switch (reader.peekChar()) {
    case '/':
        ret = CommentType::Line;
        break;
    case '*':
        ret = CommentType::Block;
        break;
    }

    reader.putBackChar('/');
    return ret;
}

size_t Parser::skipComment()
{
    Reader& reader = *_included.back();

    const CommentType commentType = getCommentType(reader);
    if (commentType == CommentType::None)
        return 0;

    SourceLocation start = reader.location();

    size_t count = 2;
    reader.getChar();
    reader.getChar();

    bool terminated = false;

    switch (commentType) {
    case CommentType::Line:
        {
            int c;
            while ((c = reader.getChar()) != Reader::kEof && c != '\r' && c != '\n')
                ++count;
            terminated = true;
        }
        break;
    case CommentType::Block:
        {
            int c1 = 0;
            int c2 = reader.getChar();
            while (!terminated && c2 != Reader::kEof) {
                c1 = c2;
                c2 = reader.getChar();
                terminated = c1 == '*' && c2 == '/';
            }
        }
        break;
    default:
        assert(false);
        break;
    }

    if (!terminated) {
        SourceLocation end = reader.location();
        emitError({ start, end }, "Unterminated block comment.");
    }

    return count;
}

void Parser::trimRight(std::string& text)
{
    while (!text.empty() && isSpaceChar(text.back()))
        text.pop_back();
}

size_t Parser::extractToEol(Reader& reader, std::string* dst)
{
    return reader.extractWhile(dst, [&reader](char c) {
        if (c == '\r' || c == '\n')
            return false;
        if (c == '/') {
            int c2 = reader.peekChar();
            if (c2 == '/' || c2 == '*') // stop at comment
                return false;
        }
        return true;
    });
}

std::string Parser::expandDollarVars(const SourceRange& range, absl::string_view src)
{
    std::string dst;
    dst.reserve(2 * src.size());

    size_t i = 0;
    size_t n = src.size();
    while (i < n) {
        char c = src[i++];

        if (c != '$')
            dst.push_back(c);
        else {
            std::string name;
            name.reserve(64);

            while (i < n && isIdentifierChar(src[i]))
                name.push_back(src[i++]);

            if (name.empty()) {
                emitWarning(range, "Expected variable name after $.");
                continue;
            }

            auto it = _currentDefinitions.find(name);
            if (it == _currentDefinitions.end()) {
                emitWarning(range, "The variable `" + name + "` is not defined.");
                continue;
            }

            dst.append(it->second);
        }
    }

    return dst;
}

bool Parser::isIdentifierChar(char c)
{
    return c == '_' ||
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9');
}

bool Parser::isSpaceChar(char c)
{
    return c == ' ' || c == '\t';
}

bool Parser::isIdentifier(absl::string_view s)
{
    if (s.empty())
        return false;

    for (char c : s) {
        if (!isIdentifierChar(c))
            return false;
    }

    return true;
}

} // namespace sfz
