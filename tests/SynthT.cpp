// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Synth.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

constexpr int blockSize { 256 };

TEST_CASE("[Synth] Play and check active voices")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    REQUIRE(synth.getNumActiveVoices() == 2);
    // Render for a while
    for (int i = 0; i < 200; ++i)
        synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 0);
}

TEST_CASE("[Synth] Change the number of voice while playing")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };
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
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");
    synth.setPreloadSize(1024);

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    synth.setPreloadSize(2048);
    synth.renderBlock(buffer);
}

TEST_CASE("[Synth] Check that we can change the oversampling factor before and after loading")
{
    sfz::Synth synth;
    synth.setOversamplingFactor(sfz::Oversampling::x2);
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");
    synth.setOversamplingFactor(sfz::Oversampling::x4);

    synth.noteOn(0, 36, 24);
    synth.noteOn(0, 36, 89);
    synth.renderBlock(buffer);
    synth.setOversamplingFactor(sfz::Oversampling::x2);
    synth.renderBlock(buffer);
}


TEST_CASE("[Synth] All notes offs/all sounds off")
{
    sfz::Synth synth;
    synth.setNumVoices(8);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/sound_off.sfz");
    synth.noteOn(0, 60, 63);
    synth.noteOn(0, 62, 63);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.cc(0, 120, 63);
    REQUIRE( synth.getNumActiveVoices() == 0 );

    synth.noteOn(0, 62, 63);
    synth.noteOn(0, 60, 63);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    synth.cc(0, 123, 63);
    REQUIRE( synth.getNumActiveVoices() == 0 );
}

TEST_CASE("[Synth] Reset all controllers")
{
    sfz::Synth synth;
    synth.cc(0, 12, 64);
    REQUIRE( synth.getMidiState().getCCValue(12) == 64 );
    synth.cc(0, 121, 64);
    REQUIRE( synth.getMidiState().getCCValue(12) == 0 );
}

TEST_CASE("[Synth] Releasing before the EG started smoothing (initial delay) kills the voice")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(1024);
    synth.setNumVoices(1);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/delay_release.sfz");
    synth.noteOn(0, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.noteOff(100, 60, 63);
    REQUIRE( synth.getVoiceView(0)->isFree() );
    synth.noteOn(200, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.noteOff(1000, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Releasing after the initial and normal mode does not trigger a fast release")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(1024);
    sfz::AudioBuffer<float> buffer(2, 1024);
    synth.setNumVoices(1);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/delay_release.sfz");
    synth.noteOn(200, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
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
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/envelope_trigger_release.sfz");
    synth.noteOn(0, 60, 63);
    synth.noteOff(0, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.renderBlock(buffer); // Attack (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Decay (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Release (0.1)
    REQUIRE( synth.getVoiceView(0)->canBeStolen() );
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
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/envelope_trigger_release_key.sfz");
    synth.noteOn(0, 60, 63);
    synth.noteOff(0, 60, 63);
    REQUIRE( !synth.getVoiceView(0)->isFree() );
    synth.renderBlock(buffer); // Attack (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Decay (0.02)
    synth.renderBlock(buffer);
    synth.renderBlock(buffer); // Release (0.1)
    REQUIRE( synth.getVoiceView(0)->canBeStolen() );
    // Release is 0.1s
    for (int i = 0; i < 10; ++i)
        synth.renderBlock(buffer);
    REQUIRE( synth.getVoiceView(0)->isFree() );
}

TEST_CASE("[Synth] Number of effect buses and resetting behavior")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };

    REQUIRE( synth.getEffectBusView(0) == nullptr); // No effects at first
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Effects/base.sfz");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_2.sfz");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    REQUIRE( synth.getEffectBusView(1) != nullptr); // and an FX bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Effects/base.sfz");
    REQUIRE( synth.getEffectBusView(0) != nullptr); // We have a main bus
    REQUIRE( synth.getEffectBusView(1) == nullptr); // and no FX bus
    // Check that we can render blocks
    for (int i = 0; i < 100; ++i)
        synth.renderBlock(buffer);

    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Effects/bitcrusher_3.sfz");
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
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Effects/base.sfz");
    auto bus = synth.getEffectBusView(0);
    REQUIRE( bus != nullptr); // We have a main bus
    REQUIRE( bus->numEffects() == 0 );
    REQUIRE( bus->gainToMain() == 1 );
    REQUIRE( bus->gainToMix() == 0 );
}
