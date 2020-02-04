// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[Opcode] Construction")
{
    SECTION("Normal construction")
    {
        sfz::Opcode opcode { "sample", "dummy" };
        REQUIRE(opcode.opcode == "sample");
        REQUIRE(opcode.lettersOnlyHash == hash("sample"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
        REQUIRE(!opcode.backParameter());
        REQUIRE(!opcode.firstParameter());
        REQUIRE(!opcode.middleParameter());
    }

    SECTION("Normal construction with underscore")
    {
        sfz::Opcode opcode { "sample_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
        REQUIRE(!opcode.backParameter());
        REQUIRE(!opcode.firstParameter());
        REQUIRE(!opcode.middleParameter());
    }

    SECTION("Parameterized opcode")
    {
        sfz::Opcode opcode { "sample123", "dummy" };
        REQUIRE(opcode.opcode == "sample123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters == std::vector<uint8_t>({ 123 }));
        REQUIRE(opcode.parameterPositions == std::vector<int>({ 6 }));
        REQUIRE(opcode.backParameter());
        REQUIRE(*opcode.backParameter() == 123);
        REQUIRE(!opcode.firstParameter());
        REQUIRE(!opcode.middleParameter());
    }

    SECTION("Parameterized opcode with underscore")
    {
        sfz::Opcode opcode { "sample_underscore123", "dummy" };
        REQUIRE(opcode.opcode == "sample_underscore123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint8_t>({ 123 }));
        REQUIRE(opcode.parameterPositions == std::vector<int>({ 17 }));
        REQUIRE(opcode.backParameter());
        REQUIRE(*opcode.backParameter() == 123);
    }

    SECTION("Parameterized opcode within the opcode")
    {
        sfz::Opcode opcode { "sample1_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample1_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint8_t>({ 1 }));
        REQUIRE(!opcode.backParameter());
        REQUIRE(opcode.firstParameter());
        REQUIRE(*opcode.firstParameter() == 1);
        REQUIRE(!opcode.middleParameter());
    }

    SECTION("Parameterized opcode within the opcode")
    {
        sfz::Opcode opcode { "sample123_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample123_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters[0] == 123);
    }

    SECTION("Parameterized opcode within the opcode twice")
    {
        sfz::Opcode opcode { "sample123_double44_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample123_double44_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_double_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 2);
        REQUIRE(opcode.parameters[0] == 123);
        REQUIRE(opcode.parameters[1] == 44);
        REQUIRE(opcode.parameters == std::vector<uint8_t>({ 123, 44 }));
        REQUIRE(opcode.parameterPositions == std::vector<int>({ 6, 13 }));
        REQUIRE(!opcode.backParameter());
        REQUIRE(opcode.firstParameter());
        REQUIRE(*opcode.firstParameter() == 123);
        REQUIRE(opcode.middleParameter());
        REQUIRE(*opcode.middleParameter() == 44);
    }

    SECTION("Parameterized opcode within the opcode twice, with a back parameter")
    {
        sfz::Opcode opcode { "sample123_double44_underscore23", "dummy" };
        REQUIRE(opcode.opcode == "sample123_double44_underscore23");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_double_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 3);
        REQUIRE(opcode.parameters == std::vector<uint8_t>({ 123, 44, 23 }));
        REQUIRE(opcode.parameterPositions == std::vector<int>({ 6, 13, 24 }));
        REQUIRE(opcode.backParameter());
        REQUIRE(*opcode.backParameter() == 23);
        REQUIRE(opcode.firstParameter());
        REQUIRE(*opcode.firstParameter() == 123);
        REQUIRE(opcode.middleParameter());
        REQUIRE(*opcode.middleParameter() == 44);
    }
}

TEST_CASE("[Opcode] Note values")
{
    auto noteValue = sfz::readNoteValue("c-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = sfz::readNoteValue("C-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = sfz::readNoteValue("g9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = sfz::readNoteValue("G9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = sfz::readNoteValue("c#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = sfz::readNoteValue("C#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
}
