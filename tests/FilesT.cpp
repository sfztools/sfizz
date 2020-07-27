// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "RegionTHelpers.h"
#include "sfizz/Synth.h"
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
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
}


TEST_CASE("[Files] Multiple regions (regions_many.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_many.sfz");
    REQUIRE(synth.getNumRegions() == 3);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy.1.wav");
    REQUIRE(synth.getRegionView(2)->sampleId.filename() == "dummy.2.wav");
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
    REQUIRE(synth.getRegionView(0)->loopMode == SfzLoopMode::loop_sustain);
}

TEST_CASE("[Files] (regions_bad.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Regions/regions_bad.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy.wav");
}

TEST_CASE("[Files] Local include")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_local.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
}

TEST_CASE("[Files] Multiple includes")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/multiple_includes.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy2.wav");
}

TEST_CASE("[Files] Multiple includes with comments")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/multiple_includes_with_comments.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy2.wav");
}

TEST_CASE("[Files] Subdir include")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_subdir.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy_subdir.wav");
}

TEST_CASE("[Files] Subdir include Win")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_subdir_win.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy_subdir.wav");
}

TEST_CASE("[Files] Recursive include (with include guard)")
{
    Synth synth;
    Parser& parser = synth.getParser();
    parser.setRecursiveIncludeGuardEnabled(true);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_recursive.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy_recursive2.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy_recursive1.wav");
}

TEST_CASE("[Files] Include loops (with include guard)")
{
    Synth synth;
    Parser& parser = synth.getParser();
    parser.setRecursiveIncludeGuardEnabled(true);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/Includes/root_loop.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy_loop2.wav");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "dummy_loop1.wav");
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
    REQUIRE(synth.getRegionView(0)->velocityRange == Range<float>(1_norm, 26_norm));
    REQUIRE(synth.getRegionView(1)->velocityRange == Range<float>(27_norm, 52_norm));
    REQUIRE(synth.getRegionView(2)->velocityRange == Range<float>(53_norm, 77_norm));
    REQUIRE(synth.getRegionView(3)->velocityRange == Range<float>(78_norm, 102_norm));
    REQUIRE(synth.getRegionView(4)->velocityRange == Range<float>(103_norm, 127_norm));
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
        REQUIRE(synth.getRegionView(0)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(1)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(2)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(4)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(5)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(6)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(7)->sampleId.filename() == "Regions/dummy.1.wav");
    }

    {
        Synth synth;
        synth.loadSfzFile(fs::current_path() / "tests/TestFiles/basic_hierarchy_antislash.sfz");
        REQUIRE(synth.getNumRegions() == 8);
        REQUIRE(synth.getRegionView(0)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(1)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(2)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(4)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(5)->sampleId.filename() == "Regions/dummy.1.wav");
        REQUIRE(synth.getRegionView(6)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(7)->sampleId.filename() == "Regions/dummy.1.wav");
    }
}

TEST_CASE("[Files] Pizz basic")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/SpecificBugs/MeatBassPizz/Programs/pizz.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    for (int i = 0; i < synth.getNumRegions(); ++i) {
        REQUIRE(synth.getRegionView(i)->keyRange == Range<uint8_t>(12, 22));
        REQUIRE(synth.getRegionView(i)->velocityRange == Range<float>(97_norm, 127_norm));
        REQUIRE(synth.getRegionView(i)->pitchKeycenter == 21);
        REQUIRE(synth.getRegionView(i)->ccConditions.getWithDefault(107) == Range<float>(0_norm, 13_norm));
    }
    REQUIRE(synth.getRegionView(0)->randRange == Range<float>(0, 0.25));
    REQUIRE(synth.getRegionView(1)->randRange == Range<float>(0.25, 0.5));
    REQUIRE(synth.getRegionView(2)->randRange == Range<float>(0.5, 0.75));
    REQUIRE(synth.getRegionView(3)->randRange == Range<float>(0.75, 1.0));
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == R"(../Samples/pizz/a0_vl4_rr1.wav)");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == R"(../Samples/pizz/a0_vl4_rr2.wav)");
    REQUIRE(synth.getRegionView(2)->sampleId.filename() == R"(../Samples/pizz/a0_vl4_rr3.wav)");
    REQUIRE(synth.getRegionView(3)->sampleId.filename() == R"(../Samples/pizz/a0_vl4_rr4.wav)");
}

TEST_CASE("[Files] Channels (channels.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/channels.sfz");
    REQUIRE(synth.getNumRegions() == 2);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "mono_sample.wav");
    REQUIRE(!synth.getRegionView(0)->isStereo());
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "stereo_sample.wav");
    REQUIRE(synth.getRegionView(1)->isStereo());
}

TEST_CASE("[Files] Channels (channels_multi.sfz)")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/channels_multi.sfz");
    REQUIRE(synth.getNumRegions() == 6);

    REQUIRE(synth.getRegionView(0)->sampleId.filename() == "*sine");
    REQUIRE(!synth.getRegionView(0)->isStereo());
    REQUIRE(synth.getRegionView(0)->isGenerator());
    REQUIRE(!synth.getRegionView(0)->oscillator);

    REQUIRE(synth.getRegionView(1)->sampleId.filename() == "*sine");
    REQUIRE(synth.getRegionView(1)->isStereo());
    REQUIRE(synth.getRegionView(1)->isGenerator());
    REQUIRE(!synth.getRegionView(1)->oscillator);

    REQUIRE(synth.getRegionView(2)->sampleId.filename() == "ramp_wave.wav");
    REQUIRE(!synth.getRegionView(2)->isStereo());
    REQUIRE(!synth.getRegionView(2)->isGenerator());
    REQUIRE(synth.getRegionView(2)->oscillator);

    REQUIRE(synth.getRegionView(3)->sampleId.filename() == "ramp_wave.wav");
    REQUIRE(synth.getRegionView(3)->isStereo());
    REQUIRE(!synth.getRegionView(3)->isGenerator());
    REQUIRE(synth.getRegionView(3)->oscillator);

    REQUIRE(synth.getRegionView(4)->sampleId.filename() == "*sine");
    REQUIRE(!synth.getRegionView(4)->isStereo());
    REQUIRE(synth.getRegionView(4)->isGenerator());
    REQUIRE(!synth.getRegionView(4)->oscillator);

    REQUIRE(synth.getRegionView(5)->sampleId.filename() == "*sine");
    REQUIRE(!synth.getRegionView(5)->isStereo());
    REQUIRE(synth.getRegionView(5)->isGenerator());
    REQUIRE(!synth.getRegionView(5)->oscillator);
}

TEST_CASE("[Files] sw_default")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/sw_default.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
}

TEST_CASE("[Files] sw_default and playing with switches")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/sw_default.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
    synth.noteOn(0, 41, 64);
    synth.noteOff(0, 41, 0);
    REQUIRE( synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(3)->isSwitchedOn() );
    synth.noteOn(0, 42, 64);
    synth.noteOff(0, 42, 0);
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(3)->isSwitchedOn() );
    synth.noteOn(0, 40, 64);
    synth.noteOff(0, 40, 64);
    REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
    REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
    REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
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
    REQUIRE(view.at(10).value == 34.0f);
}

TEST_CASE("[Files] Specific bug: relative path with backslashes")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/SpecificBugs/win_backslashes.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == R"(Xylo/Subfolder/closedhat.wav)");
}

TEST_CASE("[Files] Default path")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == R"(DefaultPath/SubPath1/sample1.wav)");
    REQUIRE(synth.getRegionView(1)->sampleId.filename() == R"(DefaultPath/SubPath2/sample2.wav)");
    REQUIRE(synth.getRegionView(2)->sampleId.filename() == R"(DefaultPath/SubPath1/sample1.wav)");
    REQUIRE(synth.getRegionView(3)->sampleId.filename() == R"(DefaultPath/SubPath2/sample2.wav)");
}

TEST_CASE("[Files] Default path reset when calling loadSfzFile again")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path.sfz");
    REQUIRE(synth.getNumRegions() == 4);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path_reset.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == R"(DefaultPath/SubPath2/sample2.wav)");
}

TEST_CASE("[Files] Default path is ignored for generators")
{
    Synth synth;
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path_generator.sfz");
    REQUIRE(synth.getNumRegions() == 1);
    REQUIRE(synth.getRegionView(0)->sampleId.filename() == R"(*sine)");
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
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/note_offset.sfz");
    REQUIRE( synth.getNumRegions() == 7 );

    REQUIRE(synth.getRegionView(0)->keyRange == Range<uint8_t>(64, 64));
    REQUIRE( synth.getRegionView(0)->pitchKeycenter == 64 );
    REQUIRE(synth.getRegionView(0)->keyswitchRange == Default::keyRange);
    REQUIRE(synth.getRegionView(0)->crossfadeKeyInRange == Default::crossfadeKeyInRange);
    REQUIRE(synth.getRegionView(0)->crossfadeKeyOutRange == Default::crossfadeKeyOutRange);

    REQUIRE(synth.getRegionView(1)->keyRange == Range<uint8_t>(51, 56));
    REQUIRE( synth.getRegionView(1)->pitchKeycenter == 51 );

    REQUIRE(synth.getRegionView(2)->keyRange == Range<uint8_t>(41, 45));
    REQUIRE( synth.getRegionView(2)->pitchKeycenter == 41 );
    REQUIRE(synth.getRegionView(2)->crossfadeKeyInRange == Range<uint8_t>(37, 41));
    REQUIRE(synth.getRegionView(2)->crossfadeKeyOutRange == Range<uint8_t>(45, 49));

    REQUIRE(synth.getRegionView(3)->keyRange == Range<uint8_t>(62, 62));
    REQUIRE(synth.getRegionView(3)->keyswitchRange == Range<uint8_t>(23, 27));
    REQUIRE( synth.getRegionView(3)->keyswitch );
    REQUIRE( *synth.getRegionView(3)->keyswitch == 24 );
    REQUIRE( synth.getRegionView(3)->keyswitchUp );
    REQUIRE( *synth.getRegionView(3)->keyswitchUp == 24 );
    REQUIRE( synth.getRegionView(3)->keyswitchDown );
    REQUIRE( *synth.getRegionView(3)->keyswitchDown == 24 );
    REQUIRE( synth.getRegionView(3)->previousNote );
    REQUIRE( *synth.getRegionView(3)->previousNote == 61 );

    REQUIRE(synth.getRegionView(4)->keyRange == Range<uint8_t>(76, 76));
    REQUIRE( synth.getRegionView(4)->pitchKeycenter == 76 );

    REQUIRE(synth.getRegionView(5)->keyRange == Range<uint8_t>(50, 50));
    REQUIRE( synth.getRegionView(5)->pitchKeycenter == 50 );

    REQUIRE(synth.getRegionView(6)->keyRange == Range<uint8_t>(50, 50));
    REQUIRE( synth.getRegionView(6)->pitchKeycenter == 50 );
}

TEST_CASE("[Files] Off by with different delays")
{
    Synth synth;
    synth.setSamplesPerBlock(256);
    AudioBuffer<float> buffer(2, 256);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/off_by.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    synth.noteOn(0, 63, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 1 );
    auto group1Voice = synth.getVoiceView(0);
    REQUIRE( group1Voice->getRegion()->group == 1ul );
    REQUIRE( group1Voice->getRegion()->offBy == 2ul );
    synth.noteOn(100, 64, 63);
    synth.renderBlock(buffer);
    REQUIRE(group1Voice->releasedOrFree());
}

TEST_CASE("[Files] Off by with the same delays")
{
    Synth synth;
    synth.setSamplesPerBlock(256);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/off_by.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    synth.noteOn(0, 63, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 1 );
    auto group1Voice = synth.getVoiceView(0);
    REQUIRE( group1Voice->getRegion()->group == 1ul );
    REQUIRE( group1Voice->getRegion()->offBy == 2ul );
    synth.noteOn(0, 64, 63);
    REQUIRE(!group1Voice->releasedOrFree());
}

TEST_CASE("[Files] Off by with the same notes at the same time")
{
    Synth synth;
    synth.setSamplesPerBlock(256);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/off_by.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    synth.noteOn(0, 65, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 2 );
    synth.noteOn(0, 65, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 4 );
    AudioBuffer<float> buffer { 2, 256 };
    synth.renderBlock(buffer);
    synth.noteOn(0, 65, 63);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices(true) == 2 );
}

TEST_CASE("[Files] Off modes")
{
    Synth synth;
    synth.setSamplesPerBlock(256);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/off_mode.sfz");
    REQUIRE( synth.getNumRegions() == 3 );
    synth.noteOn(0, 64, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 2 );
    const auto* fastVoice =
        synth.getVoiceView(0)->getRegion()->offMode == SfzOffMode::fast ?
            synth.getVoiceView(0) :
            synth.getVoiceView(1) ;
    const auto* normalVoice =
        synth.getVoiceView(0)->getRegion()->offMode == SfzOffMode::fast ?
            synth.getVoiceView(1) :
            synth.getVoiceView(0) ;
    synth.noteOn(100, 63, 63);
    REQUIRE( synth.getNumActiveVoices(true) == 3 );
    AudioBuffer<float> buffer { 2, 256 };
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices(true) == 2 );
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
    REQUIRE( synth.getRegionView(0)->loopMode == SfzLoopMode::loop_continuous );
    REQUIRE( synth.getRegionView(1)->loopMode == SfzLoopMode::no_loop );
    REQUIRE( synth.getRegionView(2)->loopMode == SfzLoopMode::loop_continuous );

    REQUIRE(synth.getRegionView(0)->loopRange == Range<uint32_t> { 77554, 186581 });
    REQUIRE(synth.getRegionView(1)->loopRange == Range<uint32_t> { 77554, 186581 });
    REQUIRE(synth.getRegionView(2)->loopRange == Range<uint32_t> { 4, 124 });
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
        REQUIRE(synth.getRegionView(0)->sampleId.filename() == "dummy1.wav");
        REQUIRE(synth.getRegionView(1)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(2)->sampleId.filename() == "Regions/dummy.wav");
        REQUIRE(synth.getRegionView(3)->sampleId.filename() == "Regions/dummy.wav");
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
    REQUIRE( ccLabels.size() == 2);
    REQUIRE( ccLabels[0].first == 54 );
    REQUIRE( ccLabels[0].second == "Gain" );
    REQUIRE( ccLabels[1].first == 2 );
    REQUIRE( ccLabels[1].second == "Other" );
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
