// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "sfizz/SfzHelpers.h"
#include <iostream>
#include "catch2/catch.hpp"
using namespace Catch::literals;

void includeTest(const std::string& line, const std::string& fileName)
{
    std::string parsedPath;
    auto found = sfz::findInclude(line, parsedPath);
    if (!found)
        std::cerr << "Include test failed: " << line << '\n';
    REQUIRE(found);
    REQUIRE(parsedPath == fileName);
}

TEST_CASE("[Parsing] #include")
{
    includeTest("#include \"file.sfz\"", "file.sfz");
    includeTest("#include \"../Programs/file.sfz\"", "../Programs/file.sfz");
    includeTest("#include \"..\\Programs\\file.sfz\"", "..\\Programs\\file.sfz");
    includeTest("#include \"file-1.sfz\"", "file-1.sfz");
    includeTest("#include \"file~1.sfz\"", "file~1.sfz");
    includeTest("#include \"file_1.sfz\"", "file_1.sfz");
    includeTest("#include \"file$1.sfz\"", "file$1.sfz");
    includeTest("#include \"file,1.sfz\"", "file,1.sfz");
    includeTest("#include \"rubbishCharactersAfter.sfz\" blabldaljf///df", "rubbishCharactersAfter.sfz");
    includeTest("#include \"lazyMatching.sfz\" b\"", "lazyMatching.sfz");
}

void defineTest(const std::string& line, const std::string& variable, const std::string& value)
{
    absl::string_view variableMatch;
    absl::string_view valueMatch;
    
    auto found = sfz::findDefine(line, variableMatch, valueMatch);
    REQUIRE(found);
    REQUIRE(variableMatch == variable);
    REQUIRE(valueMatch == value);
}

void defineFail(const std::string& line)
{
    absl::string_view variableMatch;
    absl::string_view valueMatch;
    auto found = sfz::findDefine(line, variableMatch, valueMatch);
    REQUIRE(!found);
}

TEST_CASE("[Parsing] #define")
{
    defineTest("#define $number 1", "$number", "1");
    defineTest("#define $letters QWERasdf", "$letters", "QWERasdf");
    defineTest("#define $alphanum asr1t44", "$alphanum", "asr1t44");
    defineTest("#define  $whitespace   asr1t44   ", "$whitespace", "asr1t44");
    defineTest("#define $lazyMatching  matched  bfasd ", "$lazyMatching", "matched");
    defineTest("#define $stircut  -12", "$stircut", "-12");
    defineTest("#define $_ht_under_score_  3fd", "$_ht_under_score_", "3fd");
    defineTest("#define $ht_under_score  3fd", "$ht_under_score", "3fd");
    // defineFail("#define $symbols# 1");
    // defineFail("#define $symbolsAgain $1");
    // defineFail("#define $trailingSymbols 1$");
}

TEST_CASE("[Parsing] Header")
{
    SECTION("Basic header match")
    {
        absl::string_view header;
        absl::string_view members;
        absl::string_view line { "<header>param1=value1 param2=value2<next>" };
        auto found = sfz::findHeader(line, header, members);
        REQUIRE(found);
        REQUIRE(header == "header");
        REQUIRE(members == "param1=value1 param2=value2");
        REQUIRE(line == "<next>");
    }
    SECTION("EOL header match")
    {
        absl::string_view header;
        absl::string_view members;
        absl::string_view line { "<header>param1=value1 param2=value2" };
        auto found = sfz::findHeader(line, header, members);
        REQUIRE(found);
        REQUIRE(header == "header");
        REQUIRE(members == "param1=value1 param2=value2");
        REQUIRE(line == "");
    }
}

void memberTest(const std::string& line, const std::string& opcode, const std::string& value)
{
    absl::string_view opcodeMatched;
    absl::string_view valueMatched;
    absl::string_view lineView { line };
    auto found = sfz::findOpcode(lineView, opcodeMatched, valueMatched);
    REQUIRE(found);
    REQUIRE(opcodeMatched == opcode);
    REQUIRE(valueMatched == value);
}

TEST_CASE("[Parsing] Member")
{
    memberTest("param=value", "param", "value");
    memberTest("param=113", "param", "113");
    memberTest("param1=value", "param1", "value");
    memberTest("param_1=value", "param_1", "value");
    memberTest("param_1=value", "param_1", "value");
    memberTest("ampeg_sustain_oncc74=-100", "ampeg_sustain_oncc74", "-100");
    memberTest("lorand=0.750", "lorand", "0.750");
    memberTest("sample=value", "sample", "value");
    memberTest("sample=value-()*", "sample", "value-()*");
    memberTest("sample=../sample.wav", "sample", "../sample.wav");
    memberTest("sample=..\\sample.wav", "sample", "..\\sample.wav");
    memberTest("sample=subdir\\subdir\\sample.wav", "sample", "subdir\\subdir\\sample.wav");
    memberTest("sample=subdir/subdir/sample.wav", "sample", "subdir/subdir/sample.wav");
    memberTest("sample=subdir_underscore\\sample.wav", "sample", "subdir_underscore\\sample.wav");
    memberTest("sample=subdir space\\sample.wav", "sample", "subdir space\\sample.wav");
    memberTest("sample=subdir space\\sample.wav next_member=value", "sample", "subdir space\\sample.wav");
    memberTest("sample=..\\Samples\\pizz\\a0_vl3_rr3.wav", "sample", "..\\Samples\\pizz\\a0_vl3_rr3.wav");
    memberTest("sample=..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav", "sample", "..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav");
    memberTest("sample=..\\G&S CW-Drum Kit-1\\SnareFX\\SNR-OFF-V08-CustomWorks-6x13.wav", "sample", "..\\G&S CW-Drum Kit-1\\SnareFX\\SNR-OFF-V08-CustomWorks-6x13.wav");
}