// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include "sfizz/utility/SwapAndPop.h"
using namespace Catch::literals;

TEST_CASE("[SwapAndPop] Popping one element")
{
    SECTION("PopFirst -- Test 1")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopFirst(vector, [](int v) -> bool { return v == 2; }));
        std::vector<int> expected { 1, 6, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
    SECTION("PopFirst -- Test 2")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopFirst(vector, [](int v) -> bool { return v == 5; }));
        std::vector<int> expected { 1, 2, 3, 4, 6 };
        REQUIRE( vector == expected );
    }
    SECTION("PopFirst -- Test 3")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopFirst(vector, [](int v) -> bool { return v == 1; }));
        std::vector<int> expected { 6, 2, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
    SECTION("PopFirst -- Test 4")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopFirst(vector, [](int v) -> bool { return v == 6; }));
        std::vector<int> expected { 1, 2, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 1")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 2; }) == 1);
        std::vector<int> expected { 1, 6, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 2")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 5; }) == 1);
        std::vector<int> expected { 1, 2, 3, 4, 6 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 3")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 1; }) == 1);
        std::vector<int> expected { 6, 2, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 4")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 6; }) == 1);
        std::vector<int> expected { 1, 2, 3, 4, 5 };
        REQUIRE( vector == expected );
    }
}

TEST_CASE("[SwapAndPop] Popping multiple elements")
{
    SECTION("PopAll -- Test 1")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 1 || v == 2; }) == 2);
        std::vector<int> expected { 6, 5, 3, 4 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 2")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v % 2 == 0; }) == 3);
        std::vector<int> expected { 1, 5, 3 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 3")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { return v == 1 || v == 5; }) == 2);
        std::vector<int> expected { 6, 2, 3, 4 };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 4")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        REQUIRE(sfz::swapAndPopAll(vector, [](int v) -> bool { (void)v; return true; }) == 6);
        std::vector<int> expected { };
        REQUIRE( vector == expected );
    }
    SECTION("PopAll -- Test 1")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        const auto condition = [](int v) -> bool { return v == 1 || v == 2; };
        int poppedSum = 0;
        const auto action = [&poppedSum](int v) { poppedSum += v; };
        REQUIRE(sfz::swapAndPopAll(vector, condition, action) == 2);
        std::vector<int> expected { 6, 5, 3, 4 };
        REQUIRE( vector == expected );
        REQUIRE( poppedSum == 3 );
    }
    SECTION("PopAll -- Test 2")
    {
        std::vector<int> vector { 1, 2, 3, 4, 5, 6 };
        const auto condition = [](int v) -> bool { return v % 2 == 0; };
        int poppedSum = 0;
        const auto action = [&poppedSum](int v) { poppedSum += v; };
        REQUIRE(sfz::swapAndPopAll(vector, condition, action) == 3);
        std::vector<int> expected { 1, 5, 3 };
        REQUIRE( vector == expected );
        REQUIRE( poppedSum == 12 );
    }
}
