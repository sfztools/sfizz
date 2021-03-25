// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/utility/StringViewHelpers.h"
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
