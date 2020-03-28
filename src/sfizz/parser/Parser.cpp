// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Parser.h"
#include "ParserPrivate.h"
#include "absl/memory/memory.h"

namespace sfz {

Parser::Parser()
{
}

Parser::~Parser()
{
}

void Parser::addDefinition(absl::string_view id, absl::string_view value)
{
    _definitions[id] = std::string(value);
}

void Parser::reset()
{
    _pathsIncluded.clear();
    _currentHeader.reset();
    _currentOpcodes.clear();
    _errorCount = 0;
    _warningCount = 0;
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

    includeNewFile(path, std::move(reader));
    processTopLevel();
    flushCurrentHeader();

    if (_listener)
        _listener->onParseEnd();
}

void Parser::includeNewFile(const fs::path& path, std::unique_ptr<Reader> reader)
{
    fs::path fullPath =
        (path.empty() || path.is_absolute()) ? path : _originalDirectory / path;

    if (_pathsIncluded.empty())
        _originalDirectory = fullPath.parent_path();
    else if (_pathsIncluded.find(fullPath.string()) != _pathsIncluded.end()) {
        if (_recursiveIncludeGuardEnabled)
            return;
    }

    if (_included.size() == _maxIncludeDepth) {
        SourceLocation loc;
        loc.filePath = std::make_shared<fs::path>(fullPath);
        emitError({ loc, loc }, "Exceeded maximum include depth (" + std::to_string(_maxIncludeDepth) + ")");
        return;
    }

    if (!reader) {
        auto fileReader = absl::make_unique<FileReader>(fullPath);
        if (fileReader->hasError()) {
            SourceLocation loc = fileReader->location();
            emitError({ loc, loc }, "Cannot open file for reading: " + fullPath.string());
            return;
        }
        reader = std::move(fileReader);
    }

    _pathsIncluded.insert(fullPath.string());
    _included.push_back(std::move(reader));
}

void Parser::processTopLevel()
{
    while (!_included.empty()) {
        Reader& reader = *_included.back();

        while (reader.skipChars(" \t\r\n") || skipComment(reader));

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

        if (!valid) {
            SourceLocation end = reader.location();
            emitError({ start, end }, "Expected \"file.sfz\" after #include.");
            recover();
            return;
        }

        std::replace(path.begin(), path.end(), '\\', '/');
        includeNewFile(path);
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

bool Parser::hasComment(Reader& reader)
{
    if (reader.peekChar() != '/')
        return false;

    reader.getChar();
    if (reader.peekChar() != '/') {
        reader.putBackChar('/');
        return false;
    }

    return true;
}

size_t Parser::skipComment(Reader& reader)
{
    if (!hasComment(reader))
        return 0;

    size_t count = 2;
    reader.getChar();
    reader.getChar();

    int c;
    while ((c = reader.getChar()) != Reader::kEof && c != '\r' && c != '\n')
        ++count;

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
        return c != '\r' && c != '\n' && !(c == '/' && reader.peekChar() == '/');
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

            auto it = _definitions.find(name);
            if (it == _definitions.end()) {
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
