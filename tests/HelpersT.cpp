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

#include "sfizz/StringViewHelpers.h"
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