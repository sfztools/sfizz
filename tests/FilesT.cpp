// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "TestHelpers.h"
#include "sfizz/Synth.h"
#include "sfizz/Voice.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "catch2/catch.hpp"
#include "ghc/fs_std.hpp"
#if defined(__APPLE__)
#include <unistd.h> // pathconf
#endif
using namespace Catch::literals;
using namespace sfz::literals;
using namespace sfz;

TEST_CASE("[Files] Single region (regions_one.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_one.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
}


TEST_CASE("[Files] Multiple regions (regions_many.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_many.sfz");
    REQUIRE(synth.getNumRegions() == 3);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy.1.wav");
    REQUIRE(synth.getRegionView(2)->sampleId->filename() == "dummy.2.wav");
}

TEST_CASE("[Files] Basic opcodes (regions_opcodes.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_opcodes.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->keyRange == Range<uint8_t>(2, 14));
}

TEST_CASE("[Files] Underscore opcodes (underscore_opcodes.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/underscore_opcodes.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->loopMode == LoopMode::loop_sustain);
}

TEST_CASE("[Files] (regions_bad.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_bad.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy.wav");
}

TEST_CASE("[Files] Local include")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_local.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
}

TEST_CASE("[Files] Multiple includes")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/multiple_includes.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy2.wav");
}

TEST_CASE("[Files] Multiple includes with comments")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/multiple_includes_with_comments.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy2.wav");
}

TEST_CASE("[Files] Subdir include")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_subdir.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy_subdir.wav");
}

TEST_CASE("[Files] Subdir include Win")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_subdir_win.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy_subdir.wav");
}

TEST_CASE("[Files] Recursive include (with include guard)")
{
    Synth synth;
    Parser& parser = synth.getParser();
    parser.setRecursiveIncludeGuardEnabled(true);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_recursive.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy_recursive2.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy_recursive1.wav");
}

TEST_CASE("[Files] Include loops (with include guard)")
{
    Synth synth;
    Parser& parser = synth.getParser();
    parser.setRecursiveIncludeGuardEnabled(true);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_loop.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy_loop2.wav");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "dummy_loop1.wav");
}

TEST_CASE("[Files] Define test")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/defines.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    REQUIRE(synth.getRegionView(0)->keyRange == Range<uint8_t>(36, 36));
    REQUIRE(synth.getRegionView(1)->keyRange == Range<uint8_t>(38, 38));
    REQUIRE(synth.getRegionView(2)->keyRange == Range<uint8_t>(42, 42));
    REQUIRE(synth.getRegionView(3)->volume == -12.0f);
}

TEST_CASE("[Files] Group from AVL")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/groups_avl.sfz");
    REQUIRE(synth.getNumRegions() == 5);
    for (int i = 0; i < synth.getNumRegions(); ++i) {
        REQUIRE(synth.getRegionView(i)->volume == 6.0f);
        REQUIRE(synth.getRegionView(i)->keyRange == Range<uint8_t>(36, 36));
    }

    almostEqualRanges(synth.getRegionView(0)->velocityRange, { 1_norm, 26_norm });
    almostEqualRanges(synth.getRegionView(1)->velocityRange, { 27_norm, 52_norm });
    almostEqualRanges(synth.getRegionView(2)->velocityRange, { 53_norm, 77_norm });
    almostEqualRanges(synth.getRegionView(3)->velocityRange, { 78_norm, 102_norm });
    almostEqualRanges(synth.getRegionView(4)->velocityRange, { 103_norm, 127_norm });
}

TEST_CASE("[Files] Full hierarchy")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE(synth.getNumRegions() == 8);
    for (int i = 0; i < synth.getNumRegions(); ++i) {
        REQUIRE(synth.getRegionView(i)->width == 0.4_a);
    }
    REQUIRE(synth.getRegionView(0)->pan == 0.3_a);
    REQUIRE(synth.getRegionView(0)->delay == 67);
    REQUIRE(synth.getRegionView(0)->keyRange == Range<uint8_t>(60, 60));

    REQUIRE(synth.getRegionView(1)->pan == 0.3_a);
    REQUIRE(synth.getRegionView(1)->delay == 67);
    REQUIRE(synth.getRegionView(1)->keyRange == Range<uint8_t>(61, 61));

    REQUIRE(synth.getRegionView(2)->pan == 0.3_a);
    REQUIRE(synth.getRegionView(2)->delay == 56);
    REQUIRE(synth.getRegionView(2)->keyRange == Range<uint8_t>(50, 50));

    REQUIRE(synth.getRegionView(3)->pan == 0.3_a);
    REQUIRE(synth.getRegionView(3)->delay == 56);
    REQUIRE(synth.getRegionView(3)->keyRange == Range<uint8_t>(51, 51));

    REQUIRE(synth.getRegionView(4)->pan == -0.1_a);
    REQUIRE(synth.getRegionView(4)->delay == 47);
    REQUIRE(synth.getRegionView(4)->keyRange == Range<uint8_t>(40, 40));

    REQUIRE(synth.getRegionView(5)->pan == -0.1_a);
    REQUIRE(synth.getRegionView(5)->delay == 47);
    REQUIRE(synth.getRegionView(5)->keyRange == Range<uint8_t>(41, 41));

    REQUIRE(synth.getRegionView(6)->pan == -0.1_a);
    REQUIRE(synth.getRegionView(6)->delay == 36);
    REQUIRE(synth.getRegionView(6)->keyRange == Range<uint8_t>(30, 30));

    REQUIRE(synth.getRegionView(7)->pan == -0.1_a);
    REQUIRE(synth.getRegionView(7)->delay == 36);
    REQUIRE(synth.getRegionView(7)->keyRange == Range<uint8_t>(31, 31));
}

TEST_CASE("[Files] Reloading files")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE(synth.getNumRegions() == 8);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE(synth.getNumRegions() == 8);
}

TEST_CASE("[Files] Full hierarchy with antislashes")
{
    {
        Synth synth;
        synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
        REQUIRE(synth.getNumRegions() == 8);
        REQUIRE(synth.getRegionView(0)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(1)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(2)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(4)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(5)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(6)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(7)->sampleId->filename() == "Regions/dummy.1.wav");
    }

    {
        Synth synth;
        synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy_antislash.sfz");
        REQUIRE(synth.getNumRegions() == 8);
        REQUIRE(synth.getRegionView(0)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(1)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(2)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(4)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(5)->sampleId->filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(6)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(7)->sampleId->filename() == "Regions/dummy.1.wav");
    }
}

TEST_CASE("[Files] Pizz basic")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/SpecificBugs/MeatBassPizz/Programs/pizz.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    for (int i = 0; i < synth.getNumRegions(); ++i) {
        REQUIRE(synth.getRegionView(i)->keyRange == Range<uint8_t>(12, 22));
        almostEqualRanges(synth.getRegionView(i)->velocityRange, { 97_norm, 127_norm });
        REQUIRE(synth.getRegionView(i)->pitchKeycenter == 21);
        almostEqualRanges(synth.getRegionView(i)->ccConditions.getWithDefault(107), { 0_norm, 13_norm });
    }
    almostEqualRanges(synth.getRegionView(0)->randRange, { 0, 0.25 });
    almostEqualRanges(synth.getRegionView(1)->randRange, { 0.25, 0.5 });
    almostEqualRanges(synth.getRegionView(2)->randRange, { 0.5, 0.75 });
    almostEqualRanges(synth.getRegionView(3)->randRange, { 0.75, 1.0 });
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == R"(../Samples/pizz/a0_vl4_rr1.wav)");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == R"(../Samples/pizz/a0_vl4_rr2.wav)");
    REQUIRE(synth.getRegionView(2)->sampleId->filename() == R"(../Samples/pizz/a0_vl4_rr3.wav)");
    REQUIRE(synth.getRegionView(3)->sampleId->filename() == R"(../Samples/pizz/a0_vl4_rr4.wav)");
}

TEST_CASE("[Files] Channels (channels.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/channels.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == "mono_sample.wav");
    REQUIRE(!synth.getRegionView(0)->isStereo());
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == "stereo_sample.wav");
    REQUIRE(synth.getRegionView(1)->isStereo());
}

TEST_CASE("[Files] Channels (channels_multi.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/channels_multi.sfz");
    REQUIRE(synth.getNumRegions() == 10);

    int regionNumber = 0;
    const Region* region = nullptr;

    // generator only
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "*sine");
    REQUIRE(!region->isStereo());
    REQUIRE(region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);

    // generator with multi
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "*sine");
    REQUIRE(region->isStereo());
    REQUIRE(region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);

    // explicit wavetable
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "ramp_wave.wav");
    REQUIRE(!region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::On);

    // explicit wavetable with multi
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "ramp_wave.wav");
    REQUIRE(region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::On);

    // explicit disabled wavetable
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "ramp_wave.wav");
    REQUIRE(!region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(!region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Off);

    // explicit disabled wavetable with multi
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "ramp_wave.wav");
    REQUIRE(!region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(!region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Off);

    // implicit wavetable (sound file < 3000 frames)
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "ramp_wave.wav");
    REQUIRE(!region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);

    // implicit non-wavetable (sound file >= 3000 frames)
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "snare.wav");
    REQUIRE(!region->isStereo());
    REQUIRE(!region->isGenerator());
    REQUIRE(!region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);

    // generator with multi=1 (single)
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "*sine");
    REQUIRE(!region->isStereo());
    REQUIRE(region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);

    // generator with multi=2 (ring modulation)
    region = synth.getRegionView(regionNumber++);
    REQUIRE(region->sampleId->filename() == "*sine");
    REQUIRE(!region->isStereo());
    REQUIRE(region->isGenerator());
    REQUIRE(region->isOscillator());
    REQUIRE(region->oscillatorEnabled == OscillatorEnabled::Auto);
}

TEST_CASE("[Files] wrong (overlapping) replacement for defines")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/SpecificBugs/wrong-replacements.sfz");

    REQUIRE( synth.getNumRegions() == 3 );

    // Note: test checked to be wrong under Sforzando 1.961
    //       It is the shorter matching $-variable which matches among both.
    //       The rest of the variable name creates some trailing junk text
    //       which Sforzando accepts without warning. (eg. `key=57Edge`)
    REQUIRE( synth.getRegionView(0)->keyRange.getStart() == 57 );
    REQUIRE( synth.getRegionView(0)->keyRange.getEnd() == 57 );

    REQUIRE( synth.getRegionView(1)->keyRange.getStart() == 57 );
    REQUIRE( synth.getRegionView(1)->keyRange.getEnd() == 57 );

    const ModKey target = ModKey::createNXYZ(ModId::Amplitude, synth.getRegionView(2)->getId());
    const RegionCCView view(*synth.getRegionView(2), target);
    REQUIRE(!view.empty());
    REQUIRE(view.valueAt(10) == Approx(0.34f).margin(1e-3));
}

TEST_CASE("[Files] Specific bug: relative path with backslashes")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/SpecificBugs/win_backslashes.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == R"(Xylo/Subfolder/closedhat.wav)");
}

TEST_CASE("[Files] Default path")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == R"(DefaultPath/SubPath1/sample1.wav)");
    REQUIRE(synth.getRegionView(1)->sampleId->filename() == R"(DefaultPath/SubPath2/sample2.wav)");
    REQUIRE(synth.getRegionView(2)->sampleId->filename() == R"(DefaultPath/SubPath1/sample1.wav)");
    REQUIRE(synth.getRegionView(3)->sampleId->filename() == R"(DefaultPath/SubPath2/sample2.wav)");
}

TEST_CASE("[Files] Default path reset when calling loadSfzFile again")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path_reset.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == R"(DefaultPath/SubPath2/sample2.wav)");
}

TEST_CASE("[Files] Default path is ignored for generators")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path_generator.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId->filename() == R"(*sine)");
}

TEST_CASE("[Files] Set CC applies properly")
{
    Synth synth;
    const auto& midiState = synth.getResources().midiState;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/set_cc.sfz");
    REQUIRE(midiState.getCCValue(142) == 63_norm);
    REQUIRE(midiState.getCCValue(61) == 122_norm);
}

TEST_CASE("[Files] Set HDCC applies properly")
{
    sfz::Synth synth;
    const auto& midiState = synth.getResources().midiState;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/set_hdcc.sfz");
    REQUIRE(midiState.getCCValue(142) == Approx(0.5678));
    REQUIRE(midiState.getCCValue(61) == Approx(0.1234));
}

TEST_CASE("[Files] Set RealCC applies properly")
{
    sfz::Synth synth;
    const auto& midiState = synth.getResources().midiState;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/set_realcc.sfz");
    REQUIRE(midiState.getCCValue(142) == Approx(0.5678));
    REQUIRE(midiState.getCCValue(61) == Approx(0.1234));
}

TEST_CASE("[Files] Note and octave offsets")
{
    Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/note_offset.sfz", R"(
        <control> note_offset=1
        <region> key=63 sample=*sine
        <region> lokey=50 hikey=55 pitch_keycenter=50 sample=*sine
        <region> lokey=40 hikey=44 pitch_keycenter=40 xfin_lokey=36 xfin_hikey=40 xfout_lokey=44 xfout_hikey=48 sample=*sine
        <control> note_offset=-1
        <region> key=63 sw_lokey=24 sw_hikey=28 sw_last=25 sw_up=25 sw_down=25 sw_previous=62 sample=*sine
        <control> note_offset=1 octave_offset=1
        <region> key=63 sample=*sine
        <control> note_offset=-1 octave_offset=-1
        <region> key=63 sample=*sine
        <control> // Check that this does not reset either note or octave offset
        <region> key=63 sample=*sine
    )");
    REQUIRE( synth.getNumRegions() == 7 );

    REQUIRE(synth.getRegionView(0)->keyRange == Range<uint8_t>(64, 64));
    REQUIRE( synth.getRegionView(0)->pitchKeycenter == 64 );
    REQUIRE(synth.getRegionView(0)->crossfadeKeyInRange == Default::crossfadeKeyInRange);
    REQUIRE(synth.getRegionView(0)->crossfadeKeyOutRange == Default::crossfadeKeyOutRange);

    REQUIRE(synth.getRegionView(1)->keyRange == Range<uint8_t>(51, 56));
    REQUIRE( synth.getRegionView(1)->pitchKeycenter == 51 );

    REQUIRE(synth.getRegionView(2)->keyRange == Range<uint8_t>(41, 45));
    REQUIRE( synth.getRegionView(2)->pitchKeycenter == 41 );
    REQUIRE(synth.getRegionView(2)->crossfadeKeyInRange == Range<uint8_t>(37, 41));
    REQUIRE(synth.getRegionView(2)->crossfadeKeyOutRange == Range<uint8_t>(45, 49));

    REQUIRE(synth.getRegionView(3)->keyRange == Range<uint8_t>(62, 62));
    REQUIRE( synth.getRegionView(3)->lastKeyswitch );
    REQUIRE( *synth.getRegionView(3)->lastKeyswitch == 24 );
    REQUIRE( synth.getRegionView(3)->upKeyswitch );
    REQUIRE( *synth.getRegionView(3)->upKeyswitch == 24 );
    REQUIRE( synth.getRegionView(3)->downKeyswitch );
    REQUIRE( *synth.getRegionView(3)->downKeyswitch == 24 );
    REQUIRE( synth.getRegionView(3)->previousKeyswitch );
    REQUIRE( *synth.getRegionView(3)->previousKeyswitch == 61 );

    REQUIRE(synth.getRegionView(4)->keyRange == Range<uint8_t>(76, 76));
    REQUIRE( synth.getRegionView(4)->pitchKeycenter == 76 );

    REQUIRE(synth.getRegionView(5)->keyRange == Range<uint8_t>(50, 50));
    REQUIRE( synth.getRegionView(5)->pitchKeycenter == 50 );

    REQUIRE(synth.getRegionView(6)->keyRange == Range<uint8_t>(50, 50));
    REQUIRE( synth.getRegionView(6)->pitchKeycenter == 50 );
}

TEST_CASE("[Files] Off modes")
{
    Synth synth;
    AudioBuffer<float> buffer { 2, 256 };
    synth.setSamplesPerBlock(256);

    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/off_mode.sfz");
    REQUIRE( synth.getNumRegions() == 3 );

    synth.noteOn(0, 64, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    const auto* fastVoice =
        synth.getVoiceView(0)->getRegion()->offMode == OffMode::fast ?
            synth.getVoiceView(0) :
            synth.getVoiceView(1) ;
    const auto* normalVoice =
        synth.getVoiceView(0)->getRegion()->offMode == OffMode::fast ?
            synth.getVoiceView(1) :
            synth.getVoiceView(0) ;
    synth.noteOn(100, 63, 63);
    synth.renderBlock(buffer);

    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 1 );
    for (unsigned i = 0; i < 20; ++i) // Not enough for the "normal" voice to die
        synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE( fastVoice->isFree() );
    REQUIRE( !normalVoice->isFree() );
}

TEST_CASE("[Files] Looped regions taken from files and possibly overriden")
{
    Synth synth;
    synth.setSamplesPerBlock(256);
    synth.setSampleRate(44100);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");
    REQUIRE( synth.getNumRegions() == 3 );
    REQUIRE( synth.getRegionView(0)->loopMode == LoopMode::loop_continuous );
    REQUIRE( synth.getRegionView(1)->loopMode == LoopMode::no_loop );
    REQUIRE( synth.getRegionView(2)->loopMode == LoopMode::loop_continuous );

    REQUIRE(synth.getRegionView(0)->loopRange == Range<int64_t> { 77554, 186581 });
    REQUIRE(synth.getRegionView(1)->loopRange == Range<int64_t> { 77554, 186581 });
    REQUIRE(synth.getRegionView(2)->loopRange == Range<int64_t> { 4, 124 });
}

TEST_CASE("[Files] Looped regions can start at 0")
{
    Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/loop_can_start_at_0.sfz", R"(
    <region> sample=wavetable_with_loop_at_endings.wav
    )");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->loopMode == LoopMode::loop_continuous );
    REQUIRE( synth.getRegionView(0)->loopRange == Range<int64_t> { 0, synth.getRegionView(0)->sampleEnd } );
}

TEST_CASE("[Synth] Release triggers automatically sets the loop mode")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/triggers_setting_loops.sfz", R"(
        <region> sample=kick.wav pitch_keycenter=69 loop_mode=loop_sustain trigger=release
        <region> sample=kick.wav pitch_keycenter=69 loop_mode=loop_sustain trigger=release_key
        <region> sample=kick.wav pitch_keycenter=69 trigger=release loop_mode=loop_sustain
        <region> sample=kick.wav pitch_keycenter=69 trigger=release_key loop_mode=loop_sustain
        <region> sample=looped_flute.wav pitch_keycenter=69 trigger=release_key
        <region> sample=kick.wav pitch_keycenter=69 trigger=release_key // These are normal and set to one_shot
        <region> sample=kick.wav pitch_keycenter=69 trigger=release
    )");
    REQUIRE( synth.getNumRegions() == 7 );
    REQUIRE( synth.getRegionView(0)->loopMode == LoopMode::loop_sustain );
    REQUIRE( synth.getRegionView(1)->loopMode == LoopMode::loop_sustain );
    REQUIRE( synth.getRegionView(2)->loopMode == LoopMode::loop_sustain );
    REQUIRE( synth.getRegionView(3)->loopMode == LoopMode::loop_sustain );
    REQUIRE( synth.getRegionView(4)->loopMode == LoopMode::loop_continuous );
    REQUIRE( synth.getRegionView(5)->loopMode == LoopMode::one_shot );
    REQUIRE( synth.getRegionView(6)->loopMode == LoopMode::one_shot );
}

TEST_CASE("[Files] Case sentitiveness")
{
    const fs::path sfzFilePath = fs::current_path() / "tests/TestFiles/case_insensitive.sfz";

#if defined(_WIN32)
    const bool caseSensitiveFs = false;
#elif defined(__APPLE__)
    const bool caseSensitiveFs = pathconf(sfzFilePath.string().c_str(), _PC_CASE_SENSITIVE) != 0;
#else
    const bool caseSensitiveFs = true;
#endif

    if (caseSensitiveFs) {
        Synth synth;
        synth.loadSfzFile(sfzFilePath);
        REQUIRE(synth.getNumRegions() == 4);
        REQUIRE(synth.getRegionView(0)->sampleId->filename() == "dummy1.wav");
        REQUIRE(synth.getRegionView(1)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(2)->sampleId->filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId->filename() == "Regions/dummy.wav");
    }
}

TEST_CASE("[Files] Empty file")
{
    Synth synth;
    Parser& parser = synth.getParser();
    REQUIRE(!synth.loadSfzFile(""));
    REQUIRE(parser.getIncludedFiles().empty());
    REQUIRE(!synth.loadSfzFile({}));
    REQUIRE(parser.getIncludedFiles().empty());
}

TEST_CASE("[Files] Labels")
{
    sfz::Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/labels.sfz");
    auto keyLabels = synth.getKeyLabels();
    auto ccLabels = synth.getCCLabels();
    REQUIRE( keyLabels.size() == 2);
    REQUIRE( keyLabels[0].first == 12 );
    REQUIRE( keyLabels[0].second == "Cymbals" );
    REQUIRE( keyLabels[1].first == 65 );
    REQUIRE( keyLabels[1].second == "Crash" );
    REQUIRE( ccLabels.size() >= 2);
    REQUIRE( absl::c_find(ccLabels, CCNamePair { 54, "Gain" }) != ccLabels.end() );
    REQUIRE( absl::c_find(ccLabels, CCNamePair { 2, "Other" }) != ccLabels.end() );
    const std::string xmlMidnam = synth.exportMidnam();
    REQUIRE(xmlMidnam.find("<Note Number=\"12\" Name=\"Cymbals\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Note Number=\"65\" Name=\"Crash\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"54\" Name=\"Gain\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"2\" Name=\"Other\" />") != xmlMidnam.npos);
}

TEST_CASE("[Files] Switch labels")
{
    sfz::Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/labels_sw.sfz");
    const std::string xmlMidnam = synth.exportMidnam();
    REQUIRE(xmlMidnam.find("<Note Number=\"36\" Name=\"Sine\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Note Number=\"38\" Name=\"Triangle\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Note Number=\"40\" Name=\"Saw\" />") != xmlMidnam.npos);
}

TEST_CASE("[Files] Duplicate labels")
{
    sfz::Synth synth;
    synth.loadSfzString(
        fs::current_path() / "tests/TestFiles/labels.sfz",
        R"(<control> label_key60=Baz label_key60=Quux
<control> label_cc20=Foo label_cc20=Bar
<region> sample=*sine)");

    auto keyLabels = synth.getKeyLabels();
    auto ccLabels = synth.getCCLabels();
    REQUIRE( keyLabels.size() == 1);
    REQUIRE( keyLabels[0].first == 60 );
    REQUIRE( keyLabels[0].second == "Quux" );
    REQUIRE( ccLabels.size() >= 1);
    REQUIRE( absl::c_find(ccLabels, CCNamePair { 20, "Bar" }) != ccLabels.end() );
    const std::string xmlMidnam = synth.exportMidnam();
    REQUIRE(xmlMidnam.find("<Note Number=\"60\" Name=\"Quux\" />") != xmlMidnam.npos);
    REQUIRE(xmlMidnam.find("<Control Type=\"7bit\" Number=\"20\" Name=\"Bar\" />") != xmlMidnam.npos);
}

TEST_CASE("[Files] Key center from audio file")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/sample_keycenter.sfz", R"(
        <group> pitch_keycenter=sample oscillator=off
        <region> sample=root_key_38.wav
        <region> sample=root_key_62.wav
        <region> sample=root_key_38.flac
        <region> sample=root_key_62.flac
        <region> pitch_keycenter=10 sample=root_key_62.flac
        <region> key=10 sample=root_key_62.flac
    )");

    REQUIRE(synth.getNumRegions() == 6);
    REQUIRE(synth.getRegionView(0)->pitchKeycenter == 38);
    REQUIRE(synth.getRegionView(1)->pitchKeycenter == 62);
    REQUIRE(synth.getRegionView(2)->pitchKeycenter == 38);
    REQUIRE(synth.getRegionView(3)->pitchKeycenter == 62);
    REQUIRE(synth.getRegionView(4)->pitchKeycenter == 10);
    REQUIRE(synth.getRegionView(5)->pitchKeycenter == 62);
}
