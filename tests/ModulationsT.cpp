// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "sfizz/Synth.h"
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
        REQUIRE((bool(flags & sfz::kModIsPerCycle) +
                 bool(flags & sfz::kModIsPerVoice)) == 1);
    };
    static auto* checkSourceFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        REQUIRE((bool(flags & sfz::kModIsAdditive) +
                 bool(flags & sfz::kModIsMultiplicative) +
                 bool(flags & sfz::kModIsPercentMultiplicative)) == 0);
    };
    static auto* checkTargetFlags = +[](int flags)
    {
        checkBasicFlags(flags);
        REQUIRE((bool(flags & sfz::kModIsAdditive) +
                 bool(flags & sfz::kModIsMultiplicative) +
                 bool(flags & sfz::kModIsPercentMultiplicative)) == 1);
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

TEST_CASE("[Modulations] Connection graph from SFZ")
{
    sfz::Synth synth;
    synth.loadSfzString("/modulation.sfz", R"(
<region>
sample=*sine
amplitude_oncc20=59 amplitude_curvecc20=3
pitch_oncc42=71 pitch_smoothcc42=32
pan_oncc36=14.5 pan_stepcc36=1.5
width_oncc425=29
)");
    const std::string graph = synth.getResources().modMatrix.toDotGraph();
    REQUIRE(graph == R"(digraph {
	"Controller 20 {curve=3, smooth=0, value=59, step=0}" -> "Amplitude"
	"Controller 36 {curve=0, smooth=0, value=14.5, step=1.5}" -> "Pan"
	"Controller 42 {curve=0, smooth=32, value=71, step=0}" -> "Pitch"
	"Controller 425 {curve=0, smooth=0, value=29, step=0}" -> "Width"
}
)");
}
