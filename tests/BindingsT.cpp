// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz.h"
#include "sfizz.hpp"
#include "catch2/catch.hpp"
#include "ghc/fs_std.hpp"

TEST_CASE("[Bindings] Midnam C++")
{
    sfz::Sfizz synth;
    const auto path = fs::current_path() / "tests/TestFiles/labels.sfz";
    synth.loadSfzFile(path.string());
    const std::string xmlMidnam = synth.exportMidnam("");
    REQUIRE(xmlMidnam.find("<Note Number=\"12\" Name=\"Cymbals\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Note Number=\"65\" Name=\"Crash\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"54\" Name=\"Gain\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"2\" Name=\"Other\" />") != xmlMidnam.npos);
}

TEST_CASE("[Bindings] Midnam C")
{
    sfizz_synth_t* synth = sfizz_create_synth();
    const auto path = fs::current_path() / "tests/TestFiles/labels.sfz";
    const auto strPath = path.string();
    sfizz_load_file(synth, strPath.c_str());
    char* midnamChar = sfizz_export_midnam(synth, "");
    const std::string xmlMidnam = std::string(midnamChar);
    REQUIRE(xmlMidnam.find("<Note Number=\"12\" Name=\"Cymbals\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Note Number=\"65\" Name=\"Crash\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"54\" Name=\"Gain\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"2\" Name=\"Other\" />") != xmlMidnam.npos);
    sfizz_free_memory(midnamChar);
    sfizz_free(synth);
}
