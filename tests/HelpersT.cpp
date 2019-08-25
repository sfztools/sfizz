#include "../sources/Helpers.h"
#include "catch2/catch.hpp"
#include <string_view>
using namespace Catch::literals;
using namespace std::literals::string_view_literals;

TEST_CASE("[Helpers] trimInPlace")
{
    SECTION("Trim nothing")
    {
        auto input { "view"sv };
        trimInPlace(input);
        REQUIRE(input == "view"sv);
    }

    SECTION("Trim spaces")
    {
        auto input { "   view  "sv };
        trimInPlace(input);
        REQUIRE(input == "view"sv);
    }

    SECTION("Trim other chars")
    {
        auto input { " \tview  \t"sv };
        trimInPlace(input);
        REQUIRE(input == "view"sv);
    }

    SECTION("Empty view")
    {
        auto input { "     "sv };
        trimInPlace(input);
        REQUIRE(input.empty());
    }
}

TEST_CASE("[Helpers] trim")
{
    SECTION("Trim nothing")
    {
        auto input { "view"sv };
        REQUIRE(trim(input) == "view"sv);
    }

    SECTION("Trim spaces")
    {
        auto input { "   view  "sv };
        REQUIRE(trim(input) == "view"sv);
    }

    SECTION("Trim other chars")
    {
        auto input { " \tview  \t"sv };
        REQUIRE(trim(input) == "view"sv);
    }

    SECTION("Empty view")
    {
        auto input { "     "sv };
        REQUIRE(trim(input).empty());
    }
}