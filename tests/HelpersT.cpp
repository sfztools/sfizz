// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/utility/StringViewHelpers.h"
#include "sfizz/utility/Base64.h"
#include "catch2/catch.hpp"
#include "absl/strings/string_view.h"
using namespace Catch::literals;

TEST_CASE("[Helpers] trimInPlace")
{
    SECTION("Trim nothing")
    {
        absl::string_view input { "view" };
        trimInPlace(input);
        REQUIRE(input == "view");
    }

    SECTION("Trim spaces")
    {
        absl::string_view input { "   view  " };
        trimInPlace(input);
        REQUIRE(input == "view");
    }

    SECTION("Trim other chars")
    {
        absl::string_view input { " \tview  \t" };
        trimInPlace(input);
        REQUIRE(input == "view");
    }

    SECTION("Empty view")
    {
        absl::string_view input { "     " };
        trimInPlace(input);
        REQUIRE(input.empty());
    }
}

TEST_CASE("[Helpers] trim")
{
    SECTION("Trim nothing")
    {
        absl::string_view input { "view" };
        REQUIRE(trim(input) == "view");
    }

    SECTION("Trim spaces")
    {
        absl::string_view input { "   view  " };
        REQUIRE(trim(input) == "view");
    }

    SECTION("Trim other chars")
    {
        absl::string_view input { " \tview  \t" };
        REQUIRE(trim(input) == "view");
    }

    SECTION("Empty view")
    {
        absl::string_view input { "     " };
        REQUIRE(trim(input).empty());
    }
}

TEST_CASE("[Parsing] Base64")
{
    {
        auto result = sfz::decodeBase64("");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "" );
    }

    {
        auto result = sfz::decodeBase64("Zg==");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "f" );
    }

    {
        auto result = sfz::decodeBase64("Zg");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "f" );
    }

    {
        auto result = sfz::decodeBase64("Zm8=");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "fo" );
    }

    {
        auto result = sfz::decodeBase64("Zm8");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "fo" );
    }

    {
        auto result = sfz::decodeBase64("Zm9v");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "foo" );
    }

    {
        auto result = sfz::decodeBase64("Zm9vYg==");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "foob" );
    }

    {
        auto result = sfz::decodeBase64("Zm9vYg");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "foob" );
    }

    {
        auto result = sfz::decodeBase64("Zm9vYmE=");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "fooba" );
    }

    {
        auto result = sfz::decodeBase64("Zm9vYmE");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "fooba" );
    }

    {
        auto result = sfz::decodeBase64("Zm9vYmFy");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "foobar" );
    }

    {
        auto result = sfz::decodeBase64("Zm9v\r\n Ym   \tFy");
        absl::string_view view { result.data(), result.size() };
        REQUIRE( view  == "foobar" );
    }
}
