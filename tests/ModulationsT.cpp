// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "catch2/catch.hpp"

TEST_CASE("[Modulations] Identifiers")
{
    // check that modulations are well defined as either source and target
    // and all targets have their default value defined

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        REQUIRE(sfz::ModIds::isSource(id));
        REQUIRE(!sfz::ModIds::isTarget(id));
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        REQUIRE(sfz::ModIds::isTarget(id));
        REQUIRE(!sfz::ModIds::isSource(id));
    });
}

TEST_CASE("[Modulations] Flags")
{
    // check validity of modulation flags

    static auto* checkBasicFlags = +[](int flags)
    {
        REQUIRE(flags != sfz::kModFlagsInvalid);
        REQUIRE(((flags & sfz::kModIsPerCycle) ^
                 (flags & sfz::kModIsPerVoice)) != 0);
    };
    static auto* checkSourceFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        // nothing else
    };
    static auto* checkTargetFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        REQUIRE(((flags & sfz::kModIsAdditive) ^
                 (flags & sfz::kModIsMultiplicative) ^
                 (flags & sfz::kModIsPercentMultiplicative)) != 0);
    };

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        checkSourceFlags(sfz::ModIds::flags(id));
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        checkTargetFlags(sfz::ModIds::flags(id));
    });
}

TEST_CASE("[Modulations] Display names")
{
    // check all modulations are implemented in `toString`

    sfz::ModIds::forEachSourceId([](sfz::ModId id)
    {
        REQUIRE(!sfz::ModKey(id).toString().empty());
    });

    sfz::ModIds::forEachTargetId([](sfz::ModId id)
    {
        REQUIRE(!sfz::ModKey(id).toString().empty());
    });
}
