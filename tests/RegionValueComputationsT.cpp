// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Defaults.h"
#include "sfizz/Region.h"
#include "sfizz/RegionStateful.h"
#include "sfizz/MidiState.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
#include <chrono>
#include <thread>
using namespace Catch::literals;
using namespace sfz::literals;
using namespace sfz;
constexpr int numRandomTests { 64 };

TEST_CASE("[Region] Crossfade in on key")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "3" });
    REQUIRE(noteGain(region, 2, 127_norm, midiState, curveSet) == 0.70711_a);
    REQUIRE(noteGain(region, 1, 127_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 3, 127_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on key - 2")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    REQUIRE(noteGain(region, 1, 127_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 2, 127_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 3, 127_norm, midiState, curveSet) == 0.70711_a);
    REQUIRE(noteGain(region, 4, 127_norm, midiState, curveSet) == 0.86603_a);
    REQUIRE(noteGain(region, 5, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 6, 127_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on key - gain")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE(noteGain(region, 1, 127_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 2, 127_norm, midiState, curveSet) == 0.25_a);
    REQUIRE(noteGain(region, 3, 127_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 4, 127_norm, midiState, curveSet) == 0.75_a);
    REQUIRE(noteGain(region, 5, 127_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] Crossfade out on key")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    REQUIRE(noteGain(region, 50, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 51, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 52, 127_norm, midiState, curveSet) == 0.86603_a);
    REQUIRE(noteGain(region, 53, 127_norm, midiState, curveSet) == 0.70711_a);
    REQUIRE(noteGain(region, 54, 127_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 55, 127_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 56, 127_norm, midiState, curveSet) == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on key - gain")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE(noteGain(region, 50, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 51, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 52, 127_norm, midiState, curveSet) == 0.75_a);
    REQUIRE(noteGain(region, 53, 127_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 54, 127_norm, midiState, curveSet) == 0.25_a);
    REQUIRE(noteGain(region, 55, 127_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 56, 127_norm, midiState, curveSet) == 0.0_a);
}

TEST_CASE("[Region] Crossfade in on velocity")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(noteGain(region, 1, 19_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 1, 20_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 2, 21_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 3, 22_norm, midiState, curveSet) == 0.70711_a);
    REQUIRE(noteGain(region, 4, 23_norm, midiState, curveSet) == 0.86603_a);
    REQUIRE(noteGain(region, 5, 24_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 6, 25_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on vel - gain")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(noteGain(region, 1, 19_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 1, 20_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 2, 21_norm, midiState, curveSet) == 0.25_a);
    REQUIRE(noteGain(region, 3, 22_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 4, 23_norm, midiState, curveSet) == 0.75_a);
    REQUIRE(noteGain(region, 5, 24_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 5, 25_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] Crossfade out on vel")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(noteGain(region, 5, 50_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 5, 51_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 5, 52_norm, midiState, curveSet) == 0.86603_a);
    REQUIRE(noteGain(region, 5, 53_norm, midiState, curveSet) == 0.70711_a);
    REQUIRE(noteGain(region, 5, 54_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 5, 55_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 5, 56_norm, midiState, curveSet) == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on vel - gain")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(noteGain(region, 56, 50_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 56, 51_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 56, 52_norm, midiState, curveSet) == 0.75_a);
    REQUIRE(noteGain(region, 56, 53_norm, midiState, curveSet) == 0.5_a);
    REQUIRE(noteGain(region, 56, 54_norm, midiState, curveSet) == 0.25_a);
    REQUIRE(noteGain(region, 56, 55_norm, midiState, curveSet) == 0.0_a);
    REQUIRE(noteGain(region, 56, 56_norm, midiState, curveSet) == 0.0_a);
}

TEST_CASE("[Region] Crossfade in on CC")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.5_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.70711_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.86603_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on CC - gain")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.25_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.5_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.75_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
}
TEST_CASE("[Region] Crossfade out on CC")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.86603_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.70711_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.5_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on CC - gain")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(crossfadeGain(region, midiState) == 1.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.75_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.5_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.25_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(crossfadeGain(region, midiState) == 0.0_a);
}

TEST_CASE("[Region] Velocity bug for extreme values - veltrack at 0")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(noteGain(region, 64, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 64, 0_norm, midiState, curveSet) == 1.0_a);
}


TEST_CASE("[Region] Velocity bug for extreme values - positive veltrack")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "100" });
    REQUIRE(noteGain(region, 64, 127_norm, midiState, curveSet) == 1.0_a);
    REQUIRE(noteGain(region, 64, 0_norm, midiState, curveSet) == Approx(0.0).margin(0.0001));
}

TEST_CASE("[Region] Velocity bug for extreme values - negative veltrack")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "-100" });
    REQUIRE(noteGain(region, 64, 127_norm, midiState, curveSet) == Approx(0.0).margin(0.0001));
    REQUIRE(noteGain(region, 64, 0_norm, midiState, curveSet) == 1.0_a);
}

TEST_CASE("[Region] rt_decay")
{
    MidiState midiState;
    midiState.setSampleRate(1000);
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "trigger", "release" });
    region.parseOpcode({ "rt_decay", "10" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( baseVolumedB(region, midiState, 64) == Approx(Default::volume - 1.0f).margin(0.1) );
    region.parseOpcode({ "rt_decay", "20" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( baseVolumedB(region, midiState, 64) == Approx(Default::volume - 2.0f).margin(0.1) );
    region.parseOpcode({ "trigger", "attack" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( baseVolumedB(region, midiState, 64) == Approx(Default::volume).margin(0.1) );
}

TEST_CASE("[Region] Base delay")
{
    MidiState midiState;
    Region region { 0 };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "delay", "10" });
    REQUIRE( regionDelay(region, midiState) == 10.0f );
    region.parseOpcode({ "delay_random", "10" });
    Random::randomGenerator.seed(42);
    for (int i = 0; i < numRandomTests; ++i)
    {
        auto delay = regionDelay(region, midiState);
        REQUIRE( (delay >= 10.0 && delay <= 20.0) );
    }
}

TEST_CASE("[Region] Offsets with CCs")
{
    MidiState midiState;
    Region region { 0 };

    region.parseOpcode({ "offset_cc4", "255" });
    region.parseOpcode({ "offset", "10" });
    REQUIRE( sampleOffset(region, midiState) == 10 );
    midiState.ccEvent(0, 4, 127_norm);
    REQUIRE( sampleOffset(region, midiState) == 265 );
    midiState.ccEvent(0, 4, 100_norm);
    REQUIRE( sampleOffset(region, midiState) == 210 );
    midiState.ccEvent(0, 4, 10_norm);
    REQUIRE( sampleOffset(region, midiState) == 30 );
    midiState.ccEvent(0, 4, 0);
    REQUIRE( sampleOffset(region, midiState) == 10 );
}

TEST_CASE("[Region] Pitch variation with veltrack")
{
    Region region { 0 };
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };

    REQUIRE(basePitchVariation(region, 60.0, 0_norm, midiState, curveSet) == 1.0);
    REQUIRE(basePitchVariation(region, 60.0, 64_norm, midiState, curveSet) == 1.0);
    REQUIRE(basePitchVariation(region, 60.0, 127_norm, midiState, curveSet) == 1.0);
    region.parseOpcode({ "pitch_veltrack", "1200" });
    REQUIRE(basePitchVariation(region, 60.0, 0_norm, midiState, curveSet) == 1.0);
    REQUIRE(basePitchVariation(region, 60.0, 64_norm, midiState, curveSet) == Approx(centsFactor(600.0)).margin(0.01f));
    REQUIRE(basePitchVariation(region, 60.0, 127_norm, midiState, curveSet) == Approx(centsFactor(1200.0)).margin(0.01f));
}

TEST_CASE("[Synth] velcurve")
{
    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };

    struct VelocityData { float velocity, gain; bool exact; };
    static const VelocityData veldata[] = {
        { 0_norm, 0.0, true },
        { 32_norm, 0.5f, false },
        { 64_norm, 1.0, true },
        { 96_norm, 1.0, true },
        { 127_norm, 1.0, true },
    };

    SECTION("Default veltrack")
    {
        sfz::Region region { 0 };
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "amp_velcurve_064", "1" });
        region.velCurve = Curve::buildFromVelcurvePoints(
            region.velocityPoints, Curve::Interpolator::Linear);
        for (const VelocityData& vd : veldata) {
            if (vd.exact) {
                REQUIRE(velocityCurve(region, vd.velocity, midiState, curveSet) == vd.gain);
            } else {
                REQUIRE(velocityCurve(region, vd.velocity, midiState, curveSet) == Approx(vd.gain).margin(1e-2));
            }
        }
    }

    SECTION("Inverted veltrack")
    {
        sfz::Region region { 0 };
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "amp_velcurve_064", "1" });
        region.parseOpcode({ "amp_veltrack", "-100" });
        region.velCurve = Curve::buildFromVelcurvePoints(
                region.velocityPoints, Curve::Interpolator::Linear);
        for (const VelocityData& vd : veldata) {
            if (vd.exact) {
                REQUIRE(velocityCurve(region, vd.velocity, midiState, curveSet) ==  1.0f - vd.gain);
            } else {
                REQUIRE(velocityCurve(region, vd.velocity, midiState, curveSet) == Approx( 1.0f - vd.gain).margin(1e-2));
            }
        }
    }
}

TEST_CASE("[Synth] veltrack")
{
    struct VelocityData { float velocity, dBGain; };
    struct VeltrackData { float veltrack; absl::Span<const VelocityData> veldata; };

    MidiState midiState;
    CurveSet curveSet { CurveSet::createPredefined() };

    // measured on ARIA
    const VelocityData veldata25[] = {
        { 127_norm, 0.0 },
        { 96_norm,  -1 },
        { 64_norm,  -1.8 },
        { 32_norm,  -2.3 },
        { 1_norm,   -2.5 },
    };
    const VelocityData veldata50[] = {
        { 127_norm,  0.0 },
        { 96_norm,  -2.1 },
        { 64_norm,  -4.1 },
        { 32_norm,  -5.5 },
        { 1_norm,   -6.0 },
    };
    const VelocityData veldata75[] = {
        { 127_norm,  0.0 },
        { 96_norm,  -3.4 },
        { 64_norm,  -7.2 },
        { 32_norm,  -10.5 },
        { 1_norm,   -12.0 },
    };
    const VelocityData veldata100[] = {
        { 127_norm,  0.0 },
        { 96_norm,  -4.9 },
        { 64_norm,  -12.0 },
        { 32_norm,  -24.0 },
        { 1_norm,   -84.1 },
    };

    const VeltrackData veltrackdata[] = {
        { 25, absl::MakeConstSpan(veldata25) },
        { 50, absl::MakeConstSpan(veldata50) },
        { 75, absl::MakeConstSpan(veldata75) },
        { 100, absl::MakeConstSpan(veldata100) },
    };

    for (const VeltrackData& vt : veltrackdata) {
        sfz::Region region { 0 };
        region.parseOpcode({ "sample", "*sine" });
        region.parseOpcode({ "amp_veltrack", std::to_string(vt.veltrack) });

        for (const VelocityData& vd : vt.veldata) {
            float dBGain = 20.0f * std::log10(velocityCurve(region, vd.velocity, midiState, curveSet));
            REQUIRE(dBGain == Approx(vd.dBGain).margin(0.1));
        }
    }
}
