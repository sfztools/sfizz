// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Synth.h"
#include "sfizz/Region.h"
#include "sfizz/Layer.h"
#include "sfizz/SisterVoiceRing.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/utility/NumericId.h"
#include "BitArray.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"
#include <algorithm>
using namespace Catch::literals;
using namespace sfz::literals;

// Need these for the introspection of Synth
#include "sfizz/Effects.h"

TEST_CASE("[Synth] Play and check active voices")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 2);
    // Render for a while
    for (int i = 0; i < 300; ++i)
        synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 0);
}

TEST_CASE("[Synth] All sound off")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");
    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 2);
    synth.allSoundOff();
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 0);
}

TEST_CASE("[Synth] Change the number of voice while playing")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 2);
    synth.setNumVoices(8);
    REQUIRE(synth.getNumActiveVoices() == 0);
    REQUIRE(synth.getNumVoices() == 8);
}

TEST_CASE("[Synth] Check that the sample per block and sample rate are actually propagated to all voices even on recreation")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(256);
    synth.setSampleRate(96000);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        REQUIRE( synth.getVoiceView(i)->getSamplesPerBlock() == 256 );
        REQUIRE( synth.getVoiceView(i)->getSampleRate() == 96000.0f );
    }
    synth.setNumVoices(8);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        REQUIRE( synth.getVoiceView(i)->getSamplesPerBlock() == 256 );
        REQUIRE( synth.getVoiceView(i)->getSampleRate() == 96000.0f );
    }
    synth.setSamplesPerBlock(128);
    synth.setSampleRate(48000);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        REQUIRE( synth.getVoiceView(i)->getSamplesPerBlock() == 128 );
        REQUIRE( synth.getVoiceView(i)->getSampleRate() == 48000.0f );
    }
    synth.setNumVoices(64);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        REQUIRE( synth.getVoiceView(i)->getSamplesPerBlock() == 128 );
        REQUIRE( synth.getVoiceView(i)->getSampleRate() == 48000.0f );
    }
}

TEST_CASE("[Synth] Check that we can change the size of the preload before and after loading")
{
    sfz::Synth synth;
    synth.setPreloadSize(512);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");
    synth.setPreloadSize(1024);

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    synth.setPreloadSize(2048);
    synth.renderBlock(buffer);
}

TEST_CASE("[Synth] All notes offs/all sounds off")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setNumVoices(8);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sound_off.sfz", R"(
        <region> key=60 sample=*noise
        <region> key=62 sample=*noise
    )");
    synth.noteOn(0, 60, 63);
    synth.noteOn(0, 62, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.cc(0, 120, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 62, 63);
    synth.noteOn(0, 60, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.cc(0, 123, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}

TEST_CASE("[Synth] Reset all controllers")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    const auto& midiState = synth.getResources().midiState;
    synth.cc(0, 12, 64);
    synth.renderBlock(buffer);
    REQUIRE(midiState.getCCValue(12) == 64_norm);
    synth.cc(0, 121, 64);
    synth.renderBlock(buffer);
    REQUIRE(midiState.getCCValue(12) == 0_norm);
}

TEST_CASE("[Synth] Releasing before the EG started smoothing (initial delay) kills the voice")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setNumVoices(1);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/delay_release.sfz", R"(
        <region> ampeg_delay=0.005 ampeg_release=1 sample=*noise
    )");
    synth.noteOn(0, 60, 63);
    synth.noteOff(100, 60, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getVoiceView(0)->isFree() );
    synth.noteOn(200, 60, 63);
    synth.noteOff(1000, 60, 63);
    synth.renderBlock(buffer);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Releasing after the initial and normal mode does not trigger a fast release")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer{ 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setNumVoices(1);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/delay_release.sfz", R"(
        <region> ampeg_delay=0.005 ampeg_release=1 sample=*noise
    )");
    synth.noteOn(200, 60, 63);
    synth.renderBlock(buffer);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.noteOff(0, 60, 63);
    synth.renderBlock(buffer);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Trigger=release and an envelope properly kills the voice at the end of the envelope")
{
    sfz::Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer(2, 480);
    synth.setNumVoices(1);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/envelope_trigger_release.sfz", R"(
        <group> lovel=0 hivel=127
        <region> sample=*silence
        <region> trigger=release sample=*noise loop_mode=one_shot
                 ampeg_attack=0.02 ampeg_decay=0.02 ampeg_release=0.1 ampeg_sustain=0
    )");
    synth.noteOn(0, 60, 63);
    synth.noteOff(10, 60, 63);
    synth.renderBlock(buffer); // Attack (0.02)
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Decay (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Release (0.1)
    REQUIRE(synth.getVoiceView(0)->releasedOrFree());
    // Release is 0.1s
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Trigger=release_key and an envelope properly kills the voice at the end of the envelope")
{
    sfz::Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer(2, 480);
    synth.setNumVoices(1);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/envelope_trigger_release_key.sfz", R"(
        <group> lovel=0 hivel=127
        <region> trigger=release_key sample=*noise loop_mode=one_shot
                 ampeg_attack=0.02 ampeg_decay=0.02 ampeg_release=0.1 ampeg_sustain=0
    )");
    synth.noteOn(0, 60, 63);
    synth.noteOff(10, 60, 63);
    synth.renderBlock(buffer); // Attack (0.02)
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Decay (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Release (0.1)
    REQUIRE(synth.getVoiceView(0)->releasedOrFree());
    // Release is 0.1s
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] loopmode=one_shot and an envelope properly kills the voice at the end of the envelope")
{
    sfz::Synth synth;
    synth.setSampleRate(48000);
    synth.setSamplesPerBlock(480);
    sfz::AudioBuffer<float> buffer(2, 480);
    synth.setNumVoices(1);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/envelope_one_shot.sfz", R"(
        <group> lovel=0 hivel=127
        <region> sample=*noise loop_mode=one_shot
                 ampeg_attack=0.02 ampeg_decay=0.02 ampeg_release=0.1 ampeg_sustain=0
    )");
    synth.noteOn(0, 60, 63);
    synth.noteOff(0, 60, 63);
    synth.renderBlock(buffer); // Attack (0.02)
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Decay (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Release (0.1)
    REQUIRE(synth.getVoiceView(0)->releasedOrFree());
    // Release is 0.1s
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Number of effect buses and resetting behavior")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    REQUIRE( synth.getEffectBusView(0) == nullptr); // No effects at first
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/base.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine
    )");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_2.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine effect1=100
        <effect> directtomain=50 fx1tomain=50 type=lofi bus=fx1 bitred=90 decim=10
    )");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    REQUIRE( synth.getEffectBusView(1) != nullptr); // and an FX bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/base.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine
    )");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    REQUIRE( synth.getEffectBusView(1) == nullptr); // and no FX bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_3.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine effect1=100
        <effect> directtomain=50 fx3tomain=50 type=lofi bus=fx3 bitred=90 decim=10
    )");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    REQUIRE( synth.getEffectBusView(1) == nullptr); // empty/uninitialized fx bus
    REQUIRE( synth.getEffectBusView(2) == nullptr); // empty/uninitialized fx bus
    REQUIRE( synth.getEffectBusView(3) != nullptr); // and an FX bus (because we built up to fx3)
    REQUIRE( synth.getEffectBusView(3)->numEffects() == 1);
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
}

TEST_CASE("[Synth] No effect in the main bus")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/base.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine
    )");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 0 );
    REQUIRE( bus->gainToMain() == 1 );
    REQUIRE( bus->gainToMix() == 0 );
}

TEST_CASE("[Synth] One effect")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_1.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine
        <effect> type=lofi bitred=90 decim=10
    )");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 1 );
    REQUIRE( bus->gainToMain() == 1 );
    REQUIRE( bus->gainToMix() == 0 );
}

TEST_CASE("[Synth] Effect on a second bus")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_2.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine effect1=100
        <effect> directtomain=50 fx1tomain=50 type=lofi bus=fx1 bitred=90 decim=10
    )");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 0 );
    REQUIRE( bus->gainToMain() == 0.5 );
    REQUIRE( bus->gainToMix() == 0 );
    bus = synth.getEffectBusView(1);
    REQUIRE( bus != nullptr);
    REQUIRE( bus->numEffects() == 1 );
    REQUIRE( bus->gainToMain() == 0.5 );
    REQUIRE( bus->gainToMix() == 0 );
}


TEST_CASE("[Synth] Effect on a third bus")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_3.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine effect1=100
        <effect> directtomain=50 fx3tomain=50 type=lofi bus=fx3 bitred=90 decim=10
    )");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 0 );
    REQUIRE( bus->gainToMain() == 0.5 );
    REQUIRE( bus->gainToMix() == 0 );
    bus = synth.getEffectBusView(3);
    REQUIRE( bus != nullptr);
    REQUIRE( bus->numEffects() == 1 );
    REQUIRE( bus->gainToMain() == 0.5 );
    REQUIRE( bus->gainToMix() == 0 );
}

TEST_CASE("[Synth] Gain to mix")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/Effects/to_mix.sfz", R"(
        <region> lokey=0 hikey=127 sample=*sine effect1=100
        <effect> fx1tomix=50 bus=fx1 type=lofi bitred=90 decim=10
    )");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 0 );
    REQUIRE( bus->gainToMain() == 1 );
    REQUIRE( bus->gainToMix() == 0 );
    bus = synth.getEffectBusView(1);
    REQUIRE( bus != nullptr);
    REQUIRE( bus->numEffects() == 1 );
    REQUIRE( bus->gainToMain() == 0 );
    REQUIRE( bus->gainToMix() == 0.5 );
}

TEST_CASE("[Synth] Basic curves")
{
    sfz::Synth synth;
    const auto& curves = synth.getResources().curves;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/curves.sfz", R"(
        <region> sample=*sine
        <curve>curve_index=18 v000=0 v095=0.5 v127=1
        <curve>curve_index=17 v000=0 v095=0.5 v100=1
    )");
    REQUIRE( synth.getNumCurves() == 19 );
    REQUIRE( curves.getCurve(18).evalCC7(127) == 1.0f );
    REQUIRE( curves.getCurve(18).evalCC7(95) == 0.5f );
    REQUIRE( curves.getCurve(17).evalCC7(100) == 1.0f );
    REQUIRE( curves.getCurve(17).evalCC7(95) == 0.5f );
    // Default linear
    REQUIRE( curves.getCurve(16).evalCC7(63) == Approx(63_norm) );
}

TEST_CASE("[Synth] Velocity points")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/velocity_endpoints.sfz", R"(
        <region> amp_velcurve_064=1 sample=*sine
        <region> amp_velcurve_064=1 amp_veltrack=-100 sample=*sine
    )");
    REQUIRE( !synth.getRegionView(0)->velocityPoints.empty());
    REQUIRE( synth.getRegionView(0)->velocityPoints[0].first == 64 );
    REQUIRE( synth.getRegionView(0)->velocityPoints[0].second == 1.0_a );
    REQUIRE( !synth.getRegionView(1)->velocityPoints.empty());
    REQUIRE( synth.getRegionView(1)->velocityPoints[0].first == 64 );
    REQUIRE( synth.getRegionView(1)->velocityPoints[0].second == 1.0_a );
}

TEST_CASE("[Synth] velcurve")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/velocity_endpoints.sfz", R"(
        <region> amp_velcurve_064=1 sample=*sine
        <region> amp_velcurve_064=1 amp_veltrack=-100 sample=*sine
    )");

    struct VelocityData { float velocity, gain; bool exact; };

    static const VelocityData veldata[] = {
        { 0_norm, 0.0, true },
        { 32_norm, 0.5f, false },
        { 64_norm, 1.0, true },
        { 96_norm, 1.0, true },
        { 127_norm, 1.0, true },
    };

    REQUIRE(synth.getNumRegions() == 2);
    const sfz::Region* r1 = synth.getRegionView(0);
    const sfz::Region* r2 = synth.getRegionView(1);

    for (const VelocityData& vd : veldata) {
        if (vd.exact) {
            REQUIRE(r1->velocityCurve(vd.velocity) == vd.gain);
            REQUIRE(r2->velocityCurve(vd.velocity) == 1.0f - vd.gain);
        }
        else {
            REQUIRE(r1->velocityCurve(vd.velocity) == Approx(vd.gain).margin(1e-2));
            REQUIRE(r2->velocityCurve(vd.velocity) == Approx(1.0f - vd.gain).margin(1e-2));
        }
    }
}

TEST_CASE("[Synth] veltrack")
{
    struct VelocityData { float velocity, dBGain; };
    struct VeltrackData { float veltrack; absl::Span<const VelocityData> veldata; };

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
        sfz::Synth synth;
        const std::string sfzCode = "<region>sample=*sine amp_veltrack=" +
            std::to_string(vt.veltrack);
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/veltrack.sfz", sfzCode);

        REQUIRE(synth.getNumRegions() == 1);
        const sfz::Region* r = synth.getRegionView(0);

        for (const VelocityData& vd : vt.veldata) {
            float dBGain = 20.0f * std::log10(r->velocityCurve(vd.velocity));
            REQUIRE(dBGain == Approx(vd.dBGain).margin(0.1));
        }
    }
}

TEST_CASE("[Synth] Region by identifier")
{
    sfz::Synth synth;
    synth.loadSfzString("regionByIdentifier.sfz", R"(
        <region>sample=*sine
        <region>sample=*sine
        <region>sample=doesNotExist.wav
        <region>sample=*sine
        <region>sample=doesNotExist.wav
        <region>sample=*sine
    )");

    REQUIRE(synth.getNumRegions() == 4);
    REQUIRE(synth.getRegionView(0) == synth.getRegionById(NumericId<sfz::Region>{0}));
    REQUIRE(synth.getRegionView(1) == synth.getRegionById(NumericId<sfz::Region>{1}));
    REQUIRE(nullptr == synth.getRegionById(NumericId<sfz::Region>{2}));
    REQUIRE(synth.getRegionView(2) == synth.getRegionById(NumericId<sfz::Region>{3}));
    REQUIRE(nullptr == synth.getRegionById(NumericId<sfz::Region>{4}));
    REQUIRE(synth.getRegionView(3) == synth.getRegionById(NumericId<sfz::Region>{5}));
    REQUIRE(nullptr == synth.getRegionById(NumericId<sfz::Region>{6}));
    REQUIRE(nullptr == synth.getRegionById(NumericId<sfz::Region>{}));
}

TEST_CASE("[Synth] sample quality")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString("tests/TestFiles/sampleQuality.sfz", R"(
        <region> sample=kick.wav key=60
        <region> sample=kick.wav key=61 sample_quality=5
    )");

    // default sample quality
    synth.noteOn(0, 60, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == sfz::Default::sampleQuality);
    synth.allSoundOff();

    // default sample quality, freewheeling
    synth.enableFreeWheeling();
    synth.noteOn(0, 60, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == sfz::Default::freewheelingSampleQuality);
    synth.allSoundOff();
    synth.disableFreeWheeling();

    // user-defined sample quality
    synth.setSampleQuality(sfz::Synth::ProcessLive, 3);
    synth.noteOn(0, 60, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == 3);
    synth.allSoundOff();

    // user-defined sample quality, freewheeling
    synth.enableFreeWheeling();
    synth.setSampleQuality(sfz::Synth::ProcessFreewheeling, 8);
    synth.noteOn(0, 60, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == 8);
    synth.allSoundOff();
    synth.disableFreeWheeling();

    // region sample quality
    synth.noteOn(0, 61, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == 5);
    synth.allSoundOff();

    // region sample quality, freewheeling
    synth.enableFreeWheeling();
    synth.noteOn(0, 61, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    REQUIRE(synth.getVoiceView(0)->getCurrentSampleQuality() == 5);
    synth.allSoundOff();
    synth.disableFreeWheeling();
}


TEST_CASE("[Synth] Sister voices")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sister_voices.sfz", R"(
        <region> key=61 sample=*sine
        <region> key=62 sample=*sine
        <region> key=62 sample=*sine
        <region> key=63 sample=*saw
        <region> key=63 sample=*saw
        <region> key=63 sample=*saw
    )");
    synth.noteOn(0, 61, 85);
    synth.renderBlock(buffer);
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(0)) == 1 );
    REQUIRE( synth.getVoiceView(0)->getNextSisterVoice() == synth.getVoiceView(0) );
    REQUIRE( synth.getVoiceView(0)->getPreviousSisterVoice() == synth.getVoiceView(0) );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(1)) == 2 );
    REQUIRE( synth.getVoiceView(1)->getNextSisterVoice() == synth.getVoiceView(2) );
    REQUIRE( synth.getVoiceView(1)->getPreviousSisterVoice() == synth.getVoiceView(2) );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(2)) == 2 );
    REQUIRE( synth.getVoiceView(2)->getNextSisterVoice() == synth.getVoiceView(1) );
    REQUIRE( synth.getVoiceView(2)->getPreviousSisterVoice() == synth.getVoiceView(1) );
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 6 );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(3)) == 3 );
    REQUIRE( synth.getVoiceView(3)->getNextSisterVoice() == synth.getVoiceView(4) );
    REQUIRE( synth.getVoiceView(3)->getPreviousSisterVoice() == synth.getVoiceView(5) );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(4)) == 3 );
    REQUIRE( synth.getVoiceView(4)->getNextSisterVoice() == synth.getVoiceView(5) );
    REQUIRE( synth.getVoiceView(4)->getPreviousSisterVoice() == synth.getVoiceView(3) );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(5)) == 3 );
    REQUIRE( synth.getVoiceView(5)->getNextSisterVoice() == synth.getVoiceView(3) );
    REQUIRE( synth.getVoiceView(5)->getPreviousSisterVoice() == synth.getVoiceView(4) );
}

TEST_CASE("[Synth] Apply function on sisters")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sister_voices.sfz", R"(
        <region> key=63 sample=*saw
        <region> key=63 sample=*saw
        <region> key=63 sample=*saw
    )");
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(0)) == 3 );
    float start = 1.0f;
    sfz::SisterVoiceRing::applyToRing(synth.getVoiceView(0), [&](const sfz::Voice* v) {
        start += static_cast<float>(v->getTriggerEvent().number);
    });
    REQUIRE( start == 1.0f + 3.0f * 63.0f );
}

TEST_CASE("[Synth] Sisters and off-by")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sister_voices.sfz", R"(
        <region> key=62 sample=*sine
        <group> group=1 off_by=2 <region> key=62 sample=*sine
        <group> group=2 <region> key=63 sample=*saw
    )");
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(0)) == 2 );
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    for (unsigned i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE( sfz::SisterVoiceRing::countSisterVoices(synth.getVoiceView(0)) == 1 );
}

TEST_CASE("[Synth] Release (basic behavior with sample)")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(4096);
    sfz::AudioBuffer<float> buffer { 2, 4096 };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release_key_sample.sfz", R"(
        <region> key=62 sample=*sine
        <region> key=62 sample=closedhat.wav trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine" } );
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "closedhat.wav" } );
}

TEST_CASE("[Synth] Release key (basic behavior with sample)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release_key_sample.sfz", R"(
        <region> key=62 sample=closedhat.wav trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "closedhat.wav" } );
}

TEST_CASE("[Synth] Release key (pedal)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release_key_pedal.sfz", R"(
        <region> key=62 sample=*sine trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
}

TEST_CASE("[Synth] Release")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
}

TEST_CASE("[Synth] Release (pedal was already down)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.cc(0, 64, 127);
    synth.noteOn(0, 62, 85);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
}

TEST_CASE("[Synth] Release samples don't play unless there is another playing region that matches")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.noteOff(0, 62, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.cc(0, 64, 127);
    synth.noteOn(0, 62, 85);
    synth.noteOff(0, 62, 0);
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}

TEST_CASE("[Synth] Release key (Different sustain CC)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sustain_cc=54
        <region> key=62 sample=*sine trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 54, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] Release (Different sustain CC)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sustain_cc=54
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 54, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    synth.cc(0, 54, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
}

TEST_CASE("[Synth] Release (don't check sustain)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global>sustain_cc=54 sustain_sw=off
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 54, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine" } );
}

TEST_CASE("[Synth] Release key (Different sostenuto CC)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sostenuto_cc=54
        <region> key=62 sample=*sine trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 54, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] Release (don't check sostenuto)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sostenuto_cc=54 sostenuto_sw=off
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 54, 127);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine" } );
}

TEST_CASE("[Synth] Release (Different sostenuto CC)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sostenuto_cc=54
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
        <region> key=64 sample=*silence
        <region> key=64 sample=*sine trigger=release
    )");
    SECTION("One note with sostenuto")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 54, 127);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 1 );
        synth.cc(3, 54, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 2 );
    }
    SECTION("Two notes, only one with sostenuto")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 54, 127);
        synth.noteOn(2, 64, 85);
        synth.noteOff(3, 62, 85);
        synth.noteOff(3, 64, 85);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 3 );
        synth.cc(4, 54, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 4 );
    }
}

TEST_CASE("[Synth] Release (sustain + sostenuto)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
        <region> key=64 sample=*silence
        <region> key=64 sample=*sine trigger=release
    )");
    SECTION("Sustain up first")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 66, 127);
        synth.cc(1, 64, 127);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 1 );
        synth.cc(3, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 1 );
        synth.cc(4, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 2 );
    }
    SECTION("Sostenuto up first")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 66, 127);
        synth.cc(1, 64, 127);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 1 );
        synth.cc(3, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 1 );
        synth.cc(4, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( synth.getNumActiveVoices() == 2 );
    }
}

TEST_CASE("[Synth] One shot regions with sustain + sostenuto")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/one_shot_sustain.sfz", R"(
        <region> key=60 sample=kick.wav loop_mode=one_shot
    )");
    SECTION("Sustain")
    {
        synth.noteOn(0, 60, 85);
        synth.cc(1, 64, 127);
        synth.noteOff(2, 60, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(1, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
    }
    SECTION("Sostenuto")
    {
        synth.noteOn(0, 60, 85);
        synth.cc(1, 66, 127);
        synth.noteOff(2, 60, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(1, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
    }
    SECTION("Sostenuto up first")
    {
        synth.noteOn(0, 60, 85);
        synth.cc(1, 66, 127);
        synth.cc(1, 64, 127);
        synth.noteOff(2, 60, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(3, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(4, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
    }
}

TEST_CASE("[Synth] Sustain threshold default")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 1);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}

TEST_CASE("[Synth] Sustain threshold")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <global> sustain_lo=63
        <region> key=62 sample=*silence
        <region> key=62 sample=*sine trigger=release
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 1);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine" } );
    synth.noteOn(0, 62, 85);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine", "*sine" } );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*silence", "*sine", "*sine" } );
    synth.cc(0, 64, 64);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*silence", "*sine", "*sine" } );
}

TEST_CASE("[Synth] Sustain")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sostenuto.sfz", R"(
        <region> sample=*sine
    )");

    SECTION("1 note")
    {
        synth.noteOn(0, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(1, 64, 1);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(3, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("2 notes")
    {
        synth.noteOn(0, 62, 85);
        synth.noteOn(0, 64, 85);
        synth.cc(1, 64, 1);
        synth.noteOff(2, 62, 85);
        synth.noteOff(2, 64, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 2 );
        synth.cc(3, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("1 note after the pedal is depressed")
    {
        synth.cc(0, 64, 1);
        synth.noteOn(1, 62, 85);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(3, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("2 notes, 1 after the pedal is depressed")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 64, 1);
        synth.noteOn(2, 64, 85);
        synth.noteOff(3, 62, 85);
        synth.noteOff(3, 64, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 2 );
        synth.cc(4, 64, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }
}

TEST_CASE("[Synth] Sostenuto")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sostenuto.sfz", R"(
        <region> sample=*sine
    )");

    SECTION("1 note")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 66, 1);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        synth.cc(3, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("2 notes")
    {
        synth.noteOn(0, 62, 85);
        synth.noteOn(0, 64, 85);
        synth.cc(1, 66, 1);
        synth.noteOff(2, 62, 85);
        synth.noteOff(2, 64, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 2 );
        synth.cc(3, 66, 0);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("1 note but after the pedal is depressed")
    {
        synth.cc(0, 66, 1);
        synth.noteOn(1, 62, 85);
        synth.noteOff(2, 62, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 0 );
    }

    SECTION("2 notes, 1 after the pedal is depressed")
    {
        synth.noteOn(0, 62, 85);
        synth.cc(1, 66, 1);
        synth.noteOn(2, 64, 85);
        synth.noteOff(3, 62, 85);
        synth.noteOff(3, 64, 85);
        synth.renderBlock(buffer);
        REQUIRE( numPlayingVoices(synth) == 1 );
        REQUIRE( getActiveVoices(synth)[0]->getTriggerEvent().number == 62 );
    }
}

TEST_CASE("[Synth] Release (Multiple notes, release_key ignores the pedal)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=*sine trigger=release_key
    )");
    synth.noteOn(0, 62, 85);
    synth.noteOn(0, 63, 78);
    synth.noteOn(0, 64, 34);
    synth.cc(0, 64, 127);
    synth.noteOff(0, 64, 0);
    synth.noteOff(0, 63, 2);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( activeVelocities(synth) == std::vector<float> { 34_norm, 78_norm, 85_norm} );
}

TEST_CASE("[Synth] Release (Multiple notes, release, cleared the delayed voices after)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=*silence
        <region> lokey=62 hikey=64 sample=*sine trigger=release
            loopmode=one_shot ampeg_attack=0.02 ampeg_release=0.1
    )");
    synth.noteOn(0, 62, 85);
    synth.noteOn(0, 63, 78);
    synth.noteOn(0, 64, 34);
    synth.cc(0, 64, 127);
    synth.noteOff(0, 64, 0);
    synth.noteOff(0, 63, 2);
    synth.noteOff(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    auto velocities = activeVelocities(synth);
    absl::c_sort(velocities);
    REQUIRE( velocities == std::vector<float> { 34_norm, 34_norm, 78_norm, 78_norm, 85_norm, 85_norm } );
    REQUIRE( synth.getLayerView(1)->delayedSustainReleases_.empty() );
}

TEST_CASE("[Synth] Release (Multiple notes after pedal is down, release, cleared the delayed voices after)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=*silence
        <region> lokey=62 hikey=64 sample=*sine trigger=release
            loopmode=one_shot ampeg_attack=0.02 ampeg_release=0.1
    )");
    synth.cc(0, 64, 127);
    synth.noteOn(1, 62, 85);
    synth.noteOn(1, 63, 78);
    synth.noteOn(1, 64, 34);
    synth.noteOff(2, 64, 0);
    synth.noteOff(2, 63, 2);
    synth.noteOff(2, 62, 3);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    synth.cc(3, 64, 0);
    synth.renderBlock(buffer);
    auto velocities = activeVelocities(synth);
    absl::c_sort(velocities);
    REQUIRE( velocities == std::vector<float> { 34_norm, 34_norm, 78_norm, 78_norm, 85_norm, 85_norm } );
    REQUIRE( synth.getLayerView(1)->delayedSustainReleases_.empty() );
}

TEST_CASE("[Synth] Release (Multiple note ons during pedal down)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=*silence
        <region> lokey=62 hikey=64 sample=*sine trigger=release
            loopmode=one_shot ampeg_attack=0.02 ampeg_release=0.1
    )");
    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 127);
    synth.noteOff(0, 62, 0);
    synth.noteOn(0, 62, 78);
    synth.noteOff(0, 62, 2);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    auto velocities = activeVelocities(synth);
    absl::c_sort(velocities);
    REQUIRE( velocities == std::vector<float> { 78_norm, 78_norm, 85_norm, 85_norm } );
    REQUIRE( synth.getLayerView(1)->delayedSustainReleases_.empty() );
}

TEST_CASE("[Synth] No release sample after the main sample stopped sounding by default")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(4096);
    sfz::AudioBuffer<float> buffer { 2, 4096 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=closedhat.wav loop_mode=one_shot
        <region> lokey=62 hikey=64 sample=*sine trigger=release
            loopmode=one_shot ampeg_attack=0.02 ampeg_release=0.1
    )");
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i) {
        synth.renderBlock(buffer);
    }
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOff(0, 62, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 127);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i) {
        synth.renderBlock(buffer);
    }
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOff(0, 62, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    REQUIRE( synth.getLayerView(1)->delayedSustainReleases_.empty() );
}

TEST_CASE("[Synth] If rt_dead is active the release sample can sound after the attack sample died")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(4096);
    sfz::AudioBuffer<float> buffer { 2, 4096 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/release.sfz", R"(
        <region> lokey=62 hikey=64 sample=closedhat.wav loop_mode=one_shot
        <region> lokey=62 hikey=64 sample=*sine trigger=release
            loopmode=one_shot ampeg_attack=0.02 ampeg_release=0.1
    )");
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i) {
        synth.renderBlock(buffer);
    }
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOff(0, 62, 0);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 62, 85);
    synth.cc(0, 64, 127);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    for (unsigned i = 0; i < 100; ++i) {
        synth.renderBlock(buffer);
    }
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOff(0, 62, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.cc(0, 64, 0);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    REQUIRE( synth.getLayerView(1)->delayedSustainReleases_.empty() );
}

TEST_CASE("[Synth] sw_default works at a global level")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <global> sw_default=36 sw_lokey=36 sw_hikey=39
        <region> sw_last=36 key=62 sample=*sine
        <region> sw_last=37 key=63 sample=*sine
    )");
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] sw_default works at a master level")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <master> sw_default=36 sw_lokey=36 sw_hikey=39
        <region> sw_last=36 key=62 sample=*sine
        <region> sw_last=37 key=63 sample=*sine
    )");
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] sw_default works at a group level")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <group> sw_default=36 sw_lokey=36 sw_hikey=39
        <region> sw_last=36 key=62 sample=*sine
        <region> sw_last=37 key=63 sample=*sine
    )");
    synth.noteOn(0, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] Used CCs")
{
    sfz::Synth synth;
    REQUIRE( !synth.getUsedCCs().any() );
    synth.loadSfzString(fs::current_path(), R"(
        <global> amplitude_cc1=100
        <group> volume_oncc2=5
        <region> locc4=64 hicc67=32 pan_cc5=200 sample=*sine
        <region> width_cc98=200 sample=*sine
        <region> position_cc42=200 pitch_oncc56=200 sample=*sine
        <region> start_locc44=120 hikey=-1 sample=*sine
    )");
    auto usedCCs = synth.getUsedCCs();
    REQUIRE( usedCCs.test(1) );
    REQUIRE( usedCCs.test(2) );
    REQUIRE( !usedCCs.test(3) );
    REQUIRE( usedCCs.test(4) );
    REQUIRE( usedCCs.test(5) );
    REQUIRE( !usedCCs.test(6) );
    REQUIRE( usedCCs.test(42) );
    REQUIRE( usedCCs.test(44) );
    REQUIRE( usedCCs.test(56) );
    REQUIRE( usedCCs.test(67) );
    REQUIRE( usedCCs.test(98) );
    REQUIRE( !usedCCs.test(127) );
}

TEST_CASE("[Synth] Used CCs EGs")
{
    sfz::Synth synth;
    REQUIRE( !synth.getUsedCCs().any() );
    synth.loadSfzString(fs::current_path(), R"(
        <region>
            ampeg_attack_oncc1=1
            ampeg_sustain_oncc2=2
            ampeg_start_oncc3=3
            ampeg_hold_oncc4=4
            ampeg_decay_oncc5=5
            ampeg_delay_oncc6=6
            ampeg_release_oncc7=7
            sample=*sine
        <region>
            pitcheg_attack_oncc11=11
            pitcheg_sustain_oncc12=12
            pitcheg_start_oncc13=13
            pitcheg_hold_oncc14=14
            pitcheg_decay_oncc15=15
            pitcheg_delay_oncc16=16
            pitcheg_release_oncc17=17
            sample=*sine
        <region>
            fileg_attack_oncc21=21
            fileg_sustain_oncc22=22
            fileg_start_oncc23=23
            fileg_hold_oncc24=24
            fileg_decay_oncc25=25
            fileg_delay_oncc26=26
            fileg_release_oncc27=27
            sample=*sine
    )");
    auto usedCCs = synth.getUsedCCs();
    REQUIRE( usedCCs.test(1) );
    REQUIRE( usedCCs.test(2) );
    REQUIRE( usedCCs.test(3) );
    REQUIRE( usedCCs.test(4) );
    REQUIRE( usedCCs.test(5) );
    REQUIRE( usedCCs.test(6) );
    REQUIRE( usedCCs.test(7) );
    // FIXME: enable when supported
    // REQUIRE( !usedCCs.test(8) );
    // REQUIRE( usedCCs.test(11) );
    // REQUIRE( usedCCs.test(12) );
    // REQUIRE( usedCCs.test(13) );
    // REQUIRE( usedCCs.test(14) );
    // REQUIRE( usedCCs.test(15) );
    // REQUIRE( usedCCs.test(16) );
    // REQUIRE( usedCCs.test(17) );
    // REQUIRE( !usedCCs.test(18) );
    // REQUIRE( usedCCs.test(21) );
    // REQUIRE( usedCCs.test(22) );
    // REQUIRE( usedCCs.test(23) );
    // REQUIRE( usedCCs.test(24) );
    // REQUIRE( usedCCs.test(25) );
    // REQUIRE( usedCCs.test(26) );
    // REQUIRE( usedCCs.test(27) );
    // REQUIRE( !usedCCs.test(28) );
}

TEST_CASE("[Synth] Activate also on the sustain CC")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <region> locc64=64 key=53 sample=*sine
    )");
    synth.noteOn(0, 53, 127);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.cc(1, 64, 127);
    synth.noteOn(2, 53, 127);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] Trigger also on the sustain CC")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <region> on_locc64=64 sample=*sine
    )");
    synth.cc(0, 64, 127);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
}

TEST_CASE("[Synth] end=-1 voices are immediately killed after triggering but they kill other voices")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> key=60 end=-1 sample=*sine
        <region> key=61 end=-1 sample=*silence
        <region> key=62 sample=*sine off_by=2
        <region> key=63 end=-1 sample=*saw group=2
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 61, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(1, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 0 );
}

TEST_CASE("[Synth] end=0 voices are immediately killed after triggering but they kill other voices")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> key=60 end=0 sample=*sine
        <region> key=61 end=0 sample=*silence
        <region> key=62 sample=*sine off_by=2
        <region> key=63 end=0 sample=*saw group=2
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 61, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(1, 63, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 0 );
}

TEST_CASE("[Synth] ampeg_sustain = 0 puts the ampeg envelope in free-running mode, which kills the voice almost instantly in most cases")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> key=60 sample=*sine ampeg_sustain=0
        <region> key=61 sample=*sine ampeg_sustain=0 ampeg_attack=0.1 ampeg_decay=0.1
    )");

    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
    synth.noteOn(0, 61, 85);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    // Render a bit; this does not kill the voice
    for (unsigned i = 0; i < 5; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1 );
    // Render about half a second
    for (unsigned i = 0; i < 100; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}

TEST_CASE("[Synth] Off by standard")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> group=1 off_by=2 sample=*saw transpose=12 key=60
        <region> group=2 off_by=1 sample=*triangle key=62
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(10, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    auto playingVoices = getPlayingVoices(synth);
    REQUIRE( playingVoices.front()->getRegion()->keyRange.containsWithEnd(62) );
    synth.noteOn(10, 60, 85);
    synth.renderBlock(buffer);
    playingVoices = getPlayingVoices(synth);
    REQUIRE( playingVoices.front()->getRegion()->keyRange.containsWithEnd(60) );
}

TEST_CASE("[Synth] Off by same group")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> group=1 off_by=1 sample=*saw transpose=12 key=60
        <region> group=1 off_by=1 sample=*triangle key=62
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(10, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    auto playingVoices = getPlayingVoices(synth);
    REQUIRE( playingVoices.front()->getRegion()->keyRange.containsWithEnd(62) );
    synth.noteOn(10, 60, 85);
    synth.renderBlock(buffer);
    playingVoices = getPlayingVoices(synth);
    REQUIRE( playingVoices.front()->getRegion()->keyRange.containsWithEnd(60) );
}

TEST_CASE("[Synth] Off by alone and repeated")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> group=1 off_by=1 sample=*sine key=60
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 2 );
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 3 );
}


TEST_CASE("[Synth] Off by same note and group")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> group=1 off_by=1 sample=*saw transpose=12 key=60
        <region> group=1 off_by=1 sample=*triangle key=60
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 2 );
}


TEST_CASE("[Synth] Off by with CC switches")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <group> ampeg_decay=5 ampeg_sustain=0 ampeg_release=5 key=60
        <region> sample=*saw transpose=12   group=1   off_by=2 hicc4=63
        <region> sample=*triangle           group=2   off_by=1 locc4=64
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*saw" });
    synth.cc(0, 4, 127);
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*triangle" });
    synth.cc(0, 4, 0);
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*saw" });
}

TEST_CASE("[Synth] Off by a CC event")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> group=1 off_by=2 sample=*saw
        <region> group=2 hikey=-1 on_locc67=127 on_hicc67=127 sample=*sine
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*saw" });
    synth.cc(10, 67, 127);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*sine" });
}

TEST_CASE("[Synth] Off by a note-off event")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path(), R"(
        <region> key=60 group=1 off_by=2 sample=*saw
        <region> key=62 sample=*sine
        <region> key=62 trigger=release group=2 sample=*silence
    )");
    synth.noteOn(0, 60, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*saw" });
    synth.noteOn(0, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*saw", "*sine" });
    synth.noteOff(10, 62, 85);
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "*silence" });
}

TEST_CASE("[Synth] Initial values of CC")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

    synth.loadSfzString(fs::current_path() / "init_cc.sfz", R"(
        <region> sample=*sine
    )");

    REQUIRE(synth.getHdcc(111) == 0.0f);
    REQUIRE(synth.getDefaultHdcc(111) == 0.0f);
    REQUIRE(synth.getHdcc(7) == Approx(100.0f / 127)); // default volume
    REQUIRE(synth.getDefaultHdcc(7) == Approx(100.0f / 127));
    REQUIRE(synth.getHdcc(10) == 0.5f); // default pan
    REQUIRE(synth.getDefaultHdcc(10) == 0.5f);
    REQUIRE(synth.getHdcc(11) == 1.0f); // default expression
    REQUIRE(synth.getDefaultHdcc(11) == 1.0f);

    synth.hdcc(0, 10, 0.7f);
    synth.renderBlock(buffer);
    REQUIRE(synth.getHdcc(10) == 0.7f);
    REQUIRE(synth.getDefaultHdcc(10) == 0.5f);

    synth.loadSfzString(fs::current_path() / "init_cc.sfz", R"(
        <control> set_hdcc111=0.1234 set_cc112=77
        <region> sample=*sine
    )");

    REQUIRE(synth.getHdcc(111) == Approx(0.1234f));
    REQUIRE(synth.getDefaultHdcc(111) == Approx(0.1234f));
    REQUIRE(synth.getHdcc(112) == Approx(77.0f / 127));
    REQUIRE(synth.getDefaultHdcc(112) == Approx(77.0f / 127));
}

TEST_CASE("[Synth] Default ampeg_release")
{
    sfz::Synth synth;

    synth.loadSfzString(fs::current_path() / "default_release.sfz", R"(
        <region> sample=*sine
    )");

    REQUIRE(synth.getRegionView(0)->amplitudeEG.release > 0.0005f);
}

TEST_CASE("[Synth] Send CC vs. Automate CC")
{
    {
        sfz::Synth synth;
        sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

        synth.loadSfzString(fs::current_path() / "send_cc.sfz", R"(
            <region> sample=*sine
        )");

        synth.noteOn(0, 60, 100);
        synth.hdcc(1, 120, 0.0f);

        synth.renderBlock(buffer);
        REQUIRE(synth.getNumActiveVoices() == 0);
   }

    {
        sfz::Synth synth;
        sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };

        synth.loadSfzString(fs::current_path() / "automate_cc.sfz", R"(
            <region> sample=*sine
        )");

        synth.noteOn(0, 60, 100);
        synth.automateHdcc(1, 120, 0.0f);

        synth.renderBlock(buffer);
        REQUIRE(synth.getNumActiveVoices() == 1);
   }
}

TEST_CASE("[Keyswitches] Trigger from aftertouch extended CC")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/aftertouch_trigger.sfz", R"(
        <region> start_locc129=100 start_hicc129=127 sample=*saw
    )");
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 0);
    synth.channelAftertouch(0, 90);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 0);
    synth.channelAftertouch(0, 110);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 1);
    synth.channelAftertouch(0, 120);
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 2);
}

TEST_CASE("[Synth] Short empty files are turned into *silence")
{
    sfz::Synth synth;
    std::vector<std::string> messageList;
    sfz::Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/aftertouch_trigger.sfz", R"(
        <region> sample=silence.wav
    )");
    synth.dispatchMessage(client, 0, "/region0/sample", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sample,s : { *silence }",
    };
    REQUIRE(messageList == expected);
}
