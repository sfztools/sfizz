// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/SfzHelpers.h"
#include "sfizz/parser/Parser.h"
#include <iostream>
#include "catch2/catch.hpp"
#include "absl/strings/string_view.h"
using namespace Catch::literals;

struct ParsingMocker: sfz::Parser::Listener
{
    void onParseBegin() override
    {
        beginnings++;
    }
    void onParseEnd() override
    {
        endings++;
    }
    void onParseHeader(const sfz::SourceRange&, const std::string& header) override
    {
        headers.push_back(header);
    }
    void onParseOpcode(const sfz::SourceRange&, const sfz::SourceRange&, const std::string& name, const std::string& value) override
    {
        opcodes.emplace_back(name, value);
    }
    void onParseError(const sfz::SourceRange& range, const std::string&) override
    {
        errors.push_back(range);
    }
    void onParseWarning(const sfz::SourceRange& range, const std::string&) override
    {
        warnings.push_back(range);
    }
    // high-level parsing
    void onParseFullBlock(const std::string& header, const std::vector<sfz::Opcode>& opcodes) override
    {
        fullBlockHeaders.push_back(header);
        fullBlockMembers.push_back(opcodes);
    }

    int beginnings { 0 };
    int endings { 0 };
    std::vector<sfz::SourceRange> errors;
    std::vector<sfz::SourceRange> warnings;
    std::vector<sfz::Opcode> opcodes;
    std::vector<std::string> headers;
    std::vector<std::string> fullBlockHeaders;
    std::vector<std::vector<sfz::Opcode>> fullBlockMembers;
};

TEST_CASE("[Parsing] Empty")
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/empty.sfz", "");
    REQUIRE(mock.beginnings == 1);
    REQUIRE(mock.endings == 1);
    REQUIRE(mock.errors.empty());
    REQUIRE(mock.warnings.empty());
    REQUIRE(mock.opcodes.empty());
    REQUIRE(mock.headers.empty());
    REQUIRE(mock.fullBlockHeaders.empty());
    REQUIRE(mock.fullBlockMembers.empty());
}

static const absl::string_view emptySfz {
R"(

)"
};

TEST_CASE("[Parsing] Empty2")
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/empty2.sfz", emptySfz);
    REQUIRE(mock.beginnings == 1);
    REQUIRE(mock.endings == 1);
    REQUIRE(mock.errors.empty());
    REQUIRE(mock.warnings.empty());
    REQUIRE(mock.opcodes.empty());
    REQUIRE(mock.headers.empty());
    REQUIRE(mock.fullBlockHeaders.empty());
    REQUIRE(mock.fullBlockMembers.empty());
}

namespace sfz{
bool operator==(const Opcode& lhs, const Opcode& rhs)
{
    return (lhs.name == rhs.name) && (lhs.value == rhs.value);
}
}

TEST_CASE("[Parsing] Jpcima good region")
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/goodRegion.sfz", R"(
<region> sample=*silence key=69
sample=My Directory/My Wave.wav // path with spaces and a comment
sample=My Directory/My Wave.wav key=69 // path with spaces, and other opcode following
)");
    std::vector<std::vector<sfz::Opcode>> expectedMembers = {
        {{"sample", "*silence"}, {"key", "69"}, {"sample", "My Directory/My Wave.wav"}, {"sample", "My Directory/My Wave.wav"}, {"key", "69"}}
    };
    std::vector<std::string> expectedHeaders = {
        "region"
    };
    std::vector<sfz::Opcode> expectedOpcodes;

    for (auto& members: expectedMembers)
        for (auto& opcode: members)
            expectedOpcodes.push_back(opcode);

    REQUIRE(mock.beginnings == 1);
    REQUIRE(mock.endings == 1);
    REQUIRE(mock.errors.empty());
    REQUIRE(mock.warnings.empty());
    REQUIRE(mock.opcodes == expectedOpcodes);
    REQUIRE(mock.headers == expectedHeaders);
    REQUIRE(mock.fullBlockHeaders == expectedHeaders);
    REQUIRE(mock.fullBlockMembers == expectedMembers);
}

void memberTestNew(absl::string_view member, absl::string_view opcode, absl::string_view value)
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/memberTestNew.sfz", absl::StrCat("<region> ", member));
    REQUIRE(mock.opcodes.size() == 1);
    REQUIRE(mock.headers.size() == 1);
    REQUIRE(mock.fullBlockHeaders.size() == 1);
    REQUIRE(mock.fullBlockMembers.size() == 1);
    REQUIRE(mock.headers[0] == "region");
    REQUIRE(mock.opcodes[0].name == opcode);
    REQUIRE(mock.opcodes[0].value == value);
    REQUIRE(mock.fullBlockHeaders[0] == "region");
    REQUIRE(mock.fullBlockMembers[0][0].name == opcode);
    REQUIRE(mock.fullBlockMembers[0][0].value == value);
}

TEST_CASE("[Parsing] Members (new parser)")
{
    memberTestNew("param=value", "param", "value");
    memberTestNew("param=113", "param", "113");
    memberTestNew("param1=value", "param1", "value");
    memberTestNew("param_1=value", "param_1", "value");
    memberTestNew("param_1=value", "param_1", "value");
    memberTestNew("ampeg_sustain_oncc74=-100", "ampeg_sustain_oncc74", "-100");
    memberTestNew("lorand=0.750", "lorand", "0.750");
    memberTestNew("sample=value", "sample", "value");
    memberTestNew("sample=value-()*", "sample", "value-()*");
    memberTestNew("sample=../sample.wav", "sample", "../sample.wav");
    memberTestNew("sample=..\\sample.wav", "sample", "..\\sample.wav");
    memberTestNew("sample=subdir\\subdir\\sample.wav", "sample", "subdir\\subdir\\sample.wav");
    memberTestNew("sample=subdir/subdir/sample.wav", "sample", "subdir/subdir/sample.wav");
    memberTestNew("sample=subdir_underscore\\sample.wav", "sample", "subdir_underscore\\sample.wav");
    memberTestNew("sample=subdir space\\sample.wav", "sample", "subdir space\\sample.wav");
    memberTestNew("sample=..\\Samples\\pizz\\a0_vl3_rr3.wav", "sample", "..\\Samples\\pizz\\a0_vl3_rr3.wav");
    memberTestNew("sample=..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav", "sample", "..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav");
    memberTestNew("sample=..\\G&S CW-Drum Kit-1\\SnareFX\\SNR-OFF-V08-CustomWorks-6x13.wav", "sample", "..\\G&S CW-Drum Kit-1\\SnareFX\\SNR-OFF-V08-CustomWorks-6x13.wav");
}

TEST_CASE("[Parsing] bad headers")
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/badHeaders.sfz",
R"(<>
<ab@cd> dummy_member=no
)"
    );
    REQUIRE(mock.errors.size() == 2);
    REQUIRE(mock.errors[0].start.lineNumber == 0);
    REQUIRE(mock.errors[0].start.columnNumber == 0);
    REQUIRE(mock.errors[1].start.lineNumber == 1);
    REQUIRE(mock.errors[1].start.columnNumber == 0);
    REQUIRE(mock.errors[0].end.lineNumber == 0);
    REQUIRE(mock.errors[0].end.columnNumber == 2);
    REQUIRE(mock.errors[1].end.lineNumber == 1);
    REQUIRE(mock.errors[1].end.columnNumber == 7);
}

void defineTestNew(const std::string& directive, const std::string& variable, const std::string& value)
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/defineTestNew.sfz", directive);
    const auto defines = parser.getDefines();
    REQUIRE(defines.contains(variable));
    REQUIRE(defines.at(variable) == value);
}

TEST_CASE("[Parsing] #define (new parser)")
{
    defineTestNew("#define $empty ", "empty", "");
    defineTestNew("#define $number 1", "number", "1");
    defineTestNew("#define $letters QWERasdf", "letters", "QWERasdf");
    defineTestNew("#define $alphanum asr1t44", "alphanum", "asr1t44");
    defineTestNew("#define  $whitespace   asr1t44   ", "whitespace", "asr1t44");
    // The new parser does greedy matching
    // defineTestNew("#define $lazyMatching  matched  bfasd ", "lazyMatching", "matched");
    defineTestNew("#define $stircut  -12", "stircut", "-12");
    defineTestNew("#define $_ht_under_score_  3fd", "_ht_under_score_", "3fd");
    defineTestNew("#define $ht_under_score  3fd", "ht_under_score", "3fd");
}

TEST_CASE("[Parsing] Malformed includes")
{
    sfz::Parser parser;
    ParsingMocker mock;
    parser.setListener(&mock);
    parser.parseString("/malformedIncludes.sfz",
R"(#include "MyFileWhichDoesNotExist1.sfz
#include MyFileWhichDoesNotExist1.sfz)");
    REQUIRE(mock.errors.size() == 2);
    REQUIRE(mock.errors[0].start.lineNumber == 0);
    REQUIRE(mock.errors[0].start.columnNumber == 0);
    REQUIRE(mock.errors[1].start.lineNumber == 1);
    REQUIRE(mock.errors[1].start.columnNumber == 0);
    REQUIRE(mock.errors[0].end.lineNumber == 0);
    REQUIRE(mock.errors[0].end.columnNumber == 38);
    REQUIRE(mock.errors[1].end.lineNumber == 1);
    REQUIRE(mock.errors[1].end.columnNumber == 9);
}

TEST_CASE("[Parsing] Headers (new parser)")
{
    SECTION("Basic header match")
    {
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/headers.sfz", "<header>param1=value1 param2=value2 <next>");
        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"param1", "value1"}, {"param2", "value2"}},
            {}
        };
        std::vector<std::string> expectedHeaders = {
            "header", "next"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
    }

    SECTION("EOL header match")
    {
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/eolHeaderMatch.sfz", "<header>param1=value1 param2=value2");
        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"param1", "value1"}, {"param2", "value2"}}
        };
        std::vector<std::string> expectedHeaders = {
            "header"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
    }
}

TEST_CASE("[Parsing] External definitions")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.addExternalDefinition("foo", "abc");
        parser.addExternalDefinition("bar", "123");
        parser.parseString("/externalDefinitions.sfz",
R"(<header>
param1=$foo
param2=$bar)");
        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"param1", "abc"}, {"param2", "123"}}
        };
        std::vector<std::string> expectedHeaders = {
            "header"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Block comments")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/blockComments.sfz",
R"(/* A block comment (1) */
/*
A block comment (2) */
/* A block comment (3)
*/
/* A block comment
  (4) */
/* A block comment /* // ** (5) */
)");
        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes.empty());
        REQUIRE(mock.headers.empty());
        REQUIRE(mock.fullBlockHeaders.empty());
        REQUIRE(mock.fullBlockMembers.empty());
}

TEST_CASE("[Parsing] Unterminated block comments")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/unterminatedBlockComment.sfz",
R"(/* Unterminated block comment
)");
        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.size() == 1);
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes.empty());
        REQUIRE(mock.headers.empty());
        REQUIRE(mock.fullBlockHeaders.empty());
        REQUIRE(mock.fullBlockMembers.empty());
}

TEST_CASE("[Parsing] Comments after values")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/commentsAfterValues.sfz",
R"(<header>
param1=foo param2=bar // line comment
param3=baz param4=quux /* block comment */)");
        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"param1", "foo"}, {"param2", "bar"},
             {"param3", "baz"}, {"param4", "quux"}}
        };
        std::vector<std::string> expectedHeaders = {
            "header"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Overlapping definition identifiers")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/overlappingDefinitionIdentifiers.sfz",
R"(#define $abc foo
#define $abcdef bar
<region> sample=$abc.wav
<region> sample=$abcdef.wav)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "foo.wav"}},
            {{"sample", "foodef.wav"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region", "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Interpretation of the value of #define")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/defineValues.sfz",
R"(#define $a foo #define $b bar <region> sample=$a-$b.wav
<region>#define $c toto sample=$c.wav)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "foo-bar.wav"}},
            {{"sample", "toto.wav"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region", "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Recursive expansion")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/recursiveExpansion.sfz",
R"(#define $B foo-$A-baz
#define $A bar
<region> sample=$B.wav)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "foo-bar-baz.wav"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Strange #define behavior")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/define_cc_bug.sfz",R"(
#define $CCVAL 100
<region> amplitude_oncc$CCVAL=100 sample=*sine
<region> transpose=-12 amplitude_oncc$CCVAL=100 sample=*sine
<region> transpose=-12 amplitude_oncc100=100 sample=*sine
)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"amplitude_oncc100", "100"},
             {"sample", "*sine"}},
            {{"transpose", "-12"},
             {"amplitude_oncc100", "100"},
             {"sample", "*sine"}},
            {{"transpose", "-12"},
             {"amplitude_oncc100", "100"},
             {"sample", "*sine"}},
        };

        std::vector<std::string> expectedHeaders = {
            "region", "region", "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Opcode value special character")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/opcodeValueSpecialCharacter.sfz",
R"(<region>
sample=Alto-Flute-sus-C#4-PB-loop.wav
<region>
sample=foo=bar<group>)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "Alto-Flute-sus-C#4-PB-loop.wav"}},
            {{"sample", "foo=bar"}},
            {},
        };
        std::vector<std::string> expectedHeaders = {
            "region", "region", "group"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Opcode value with inline directives")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/opcodeValueWithInlineDirective.sfz",
R"(
<region>#define $VEL v1 sample=$VEL.wav #define $FOO bar
<region>#define $VEL v2 sample=$VEL.wav #notAdirective 123
)");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "v1.wav"}},
            {{"sample", "v2.wav #notAdirective 123"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region", "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Opcode value with multiple consecutive spaces")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseString("/opcodeValueWithMultipleConsecutiveSpaces.sfz",
R"(<region>  sample=foo  bar  baz  .wav   key=69  )");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "foo  bar  baz  .wav"},
             {"key", "69"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}

TEST_CASE("[Parsing] Expanded value of #include")
{
        sfz::Parser parser;
        ParsingMocker mock;
        parser.setListener(&mock);
        parser.parseFile(fs::current_path() / "tests/TestFiles/dollar_include_main.sfz");

        std::vector<std::vector<sfz::Opcode>> expectedMembers = {
            {{"sample", "*sine"}},
        };
        std::vector<std::string> expectedHeaders = {
            "region"
        };
        std::vector<sfz::Opcode> expectedOpcodes;

        for (auto& members: expectedMembers)
            for (auto& opcode: members)
                expectedOpcodes.push_back(opcode);

        REQUIRE(mock.beginnings == 1);
        REQUIRE(mock.endings == 1);
        REQUIRE(mock.errors.empty());
        REQUIRE(mock.warnings.empty());
        REQUIRE(mock.opcodes == expectedOpcodes);
        REQUIRE(mock.headers == expectedHeaders);
        REQUIRE(mock.fullBlockHeaders == expectedHeaders);
        REQUIRE(mock.fullBlockMembers == expectedMembers);
}
