// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include "sfizz/SfzHelpers.h"
#include <set>

template<class F>
void checkAllCCs(const std::set<int>& validCCs, F&& check)
{
    for (int cc = 0; cc < sfz::config::numCCs; ++cc) {
        INFO("The number is " << cc);
        if (validCCs.find(cc) != validCCs.end())
            REQUIRE(check(cc));
        else
            REQUIRE_FALSE(check(cc));
    }
}

TEST_CASE("[CC] Extended CCs")
{
    std::set<int> supportedExtendedCCs { 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 140, 141};
    checkAllCCs(supportedExtendedCCs, sfz::isExtendedCC);
}

TEST_CASE("[CC] ARIA Extended CCs")
{
    std::set<int> supportedAriaExtendedCCs { 140, 141 };
    checkAllCCs(supportedAriaExtendedCCs, sfz::isAriaExtendedCC);
}

TEST_CASE("[CC] Per-Voice Extended CCs")
{
    std::set<int> perVoiceSupportedCCs { 131, 132, 133, 134, 135, 136, 137, 140, 141 };
    checkAllCCs(perVoiceSupportedCCs, sfz::ccModulationIsPerVoice);
}

