#include "catch2/catch.hpp"
#include "../sources/Synth.h"
#include <filesystem>
using namespace Catch::literals;

TEST_CASE("[Files] Single region (regions_one.sfz)")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Regions/regions_one.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy.wav" );
}

TEST_CASE("[Files] Multiple regions (regions_many.sfz)")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Regions/regions_many.sfz");
    REQUIRE( synth.getNumRegions() == 3 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy.wav" );
    REQUIRE( synth.getRegionView(1)->sample == "dummy.1.wav" );
    REQUIRE( synth.getRegionView(2)->sample == "dummy.2.wav" );
}

TEST_CASE("[Files] Basic opcodes (regions_opcodes.sfz)")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Regions/regions_opcodes.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->channelRange == Range<uint8_t>(2, 14) );  
}

TEST_CASE("[Files] Underscore opcodes (underscore_opcodes.sfz)")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Regions/underscore_opcodes.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->loopMode == SfzLoopMode::loop_sustain );  
}

TEST_CASE("[Files] Local include")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/root_local.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy.wav" );
}

TEST_CASE("[Files] Multiple includes")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/multiple_includes.sfz");
    REQUIRE( synth.getNumRegions() == 2 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy.wav" );
    REQUIRE( synth.getRegionView(1)->sample == "dummy2.wav" );
}

TEST_CASE("[Files] Multiple includes with comments")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/multiple_includes_with_comments.sfz");
    REQUIRE( synth.getNumRegions() == 2 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy.wav" );
    REQUIRE( synth.getRegionView(1)->sample == "dummy2.wav" );
}

TEST_CASE("[Files] Subdir include")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/root_subdir.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy_subdir.wav" );
}

TEST_CASE("[Files] Subdir include Win")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/root_subdir_win.sfz");
    REQUIRE( synth.getNumRegions() == 1 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy_subdir.wav" );
}

TEST_CASE("[Files] Recursive include")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/root_recursive.sfz");
    REQUIRE( synth.getNumRegions() == 2 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy_recursive2.wav" );
    REQUIRE( synth.getRegionView(1)->sample == "dummy_recursive1.wav" );
}

TEST_CASE("[Files] Include loops")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/Includes/root_loop.sfz");
    REQUIRE( synth.getNumRegions() == 2 );
    REQUIRE( synth.getRegionView(0)->sample == "dummy_loop2.wav" );
    REQUIRE( synth.getRegionView(1)->sample == "dummy_loop1.wav" );
}

TEST_CASE("[Files] Define test")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/defines.sfz");
    REQUIRE( synth.getNumRegions() == 3 );
    REQUIRE( synth.getRegionView(0)->keyRange == Range<uint8_t>(36, 36) );
    REQUIRE( synth.getRegionView(1)->keyRange == Range<uint8_t>(38, 38) );
    REQUIRE( synth.getRegionView(2)->keyRange == Range<uint8_t>(42, 42) );
}


TEST_CASE("[Files] Group from AVL")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/groups_avl.sfz");
    REQUIRE( synth.getNumRegions() == 5 );
    for (int i = 0; i < synth.getNumRegions(); ++i)
    {
        REQUIRE( synth.getRegionView(i)->volume == 6.0f );
        REQUIRE( synth.getRegionView(i)->keyRange == Range<uint8_t>(36, 36) );
    }
    REQUIRE( synth.getRegionView(0)->velocityRange == Range<uint8_t>(1, 26) );
    REQUIRE( synth.getRegionView(1)->velocityRange == Range<uint8_t>(27, 52) );
    REQUIRE( synth.getRegionView(2)->velocityRange == Range<uint8_t>(53, 77) );
    REQUIRE( synth.getRegionView(3)->velocityRange == Range<uint8_t>(78, 102) );
    REQUIRE( synth.getRegionView(4)->velocityRange == Range<uint8_t>(103, 127) );
}

TEST_CASE("[Files] Full hierarchy")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE( synth.getNumRegions() == 8 );
    for (int i = 0; i < synth.getNumRegions(); ++i)
    {
        REQUIRE( synth.getRegionView(i)->width == 40.0f );
    }
    REQUIRE( synth.getRegionView(0)->pan == 30.0f );
    REQUIRE( synth.getRegionView(0)->delay == 67 );
    REQUIRE( synth.getRegionView(0)->keyRange == Range<uint8_t>(60, 60) );

    REQUIRE( synth.getRegionView(1)->pan == 30.0f );
    REQUIRE( synth.getRegionView(1)->delay == 67 );
    REQUIRE( synth.getRegionView(1)->keyRange == Range<uint8_t>(61, 61) );

    REQUIRE( synth.getRegionView(2)->pan == 30.0f );
    REQUIRE( synth.getRegionView(2)->delay == 56 );
    REQUIRE( synth.getRegionView(2)->keyRange == Range<uint8_t>(50, 50) );

    REQUIRE( synth.getRegionView(3)->pan == 30.0f );
    REQUIRE( synth.getRegionView(3)->delay == 56 );
    REQUIRE( synth.getRegionView(3)->keyRange == Range<uint8_t>(51, 51) );

    REQUIRE( synth.getRegionView(4)->pan == -10.0f );
    REQUIRE( synth.getRegionView(4)->delay == 47 );
    REQUIRE( synth.getRegionView(4)->keyRange == Range<uint8_t>(40, 40) );

    REQUIRE( synth.getRegionView(5)->pan == -10.0f );
    REQUIRE( synth.getRegionView(5)->delay == 47 );
    REQUIRE( synth.getRegionView(5)->keyRange == Range<uint8_t>(41, 41) );

    REQUIRE( synth.getRegionView(6)->pan == -10.0f );
    REQUIRE( synth.getRegionView(6)->delay == 36 );
    REQUIRE( synth.getRegionView(6)->keyRange == Range<uint8_t>(30, 30) );

    REQUIRE( synth.getRegionView(7)->pan == -10.0f );
    REQUIRE( synth.getRegionView(7)->delay == 36 );
    REQUIRE( synth.getRegionView(7)->keyRange == Range<uint8_t>(31, 31) );
}

TEST_CASE("[Files] Reloading files")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE( synth.getNumRegions() == 8 );
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
    REQUIRE( synth.getNumRegions() == 8 );
}

TEST_CASE("[Files] Full hierarchy with antislashes")
{
    {
        sfz::Synth synth;
        synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/basic_hierarchy.sfz");
        REQUIRE( synth.getNumRegions() == 8 );
        REQUIRE( synth.getRegionView(0)->sample == "Regions/dummy.wav" );
        REQUIRE( synth.getRegionView(1)->sample == "Regions/dummy.1.wav" );
        REQUIRE( synth.getRegionView(2)->sample == "Regions/dummy.wav" );
        REQUIRE( synth.getRegionView(3)->sample == "Regions/dummy.1.wav" );
        REQUIRE( synth.getRegionView(4)->sample == "Regions/dummy.wav"  );
        REQUIRE( synth.getRegionView(5)->sample == "Regions/dummy.1.wav"  );
        REQUIRE( synth.getRegionView(6)->sample == "Regions/dummy.wav"  );
        REQUIRE( synth.getRegionView(7)->sample == "Regions/dummy.1.wav"  );
    }

    {
        sfz::Synth synth;
        synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/basic_hierarchy_antislash.sfz");
        REQUIRE( synth.getNumRegions() == 8 );
        REQUIRE( synth.getRegionView(0)->sample == "Regions/dummy.wav" );
        REQUIRE( synth.getRegionView(1)->sample == "Regions/dummy.1.wav" );
        REQUIRE( synth.getRegionView(2)->sample == "Regions/dummy.wav" );
        REQUIRE( synth.getRegionView(3)->sample == "Regions/dummy.1.wav" );
        REQUIRE( synth.getRegionView(4)->sample == "Regions/dummy.wav"  );
        REQUIRE( synth.getRegionView(5)->sample == "Regions/dummy.1.wav"  );
        REQUIRE( synth.getRegionView(6)->sample == "Regions/dummy.wav"  );
        REQUIRE( synth.getRegionView(7)->sample == "Regions/dummy.1.wav"  );
    }
}

TEST_CASE("[Files] Pizz basic")
{
    sfz::Synth synth;
    synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/SpecificBugs/MeatBassPizz/Programs/pizz.sfz");
    REQUIRE( synth.getNumRegions() == 4 );
    for (int i = 0; i < synth.getNumRegions(); ++i)
    {
        REQUIRE( synth.getRegionView(i)->keyRange == Range<uint8_t>(12, 22) );
        REQUIRE( synth.getRegionView(i)->velocityRange == Range<uint8_t>(97, 127) );
        REQUIRE( synth.getRegionView(i)->pitchKeycenter == 21 );
        REQUIRE( synth.getRegionView(i)->ccConditions.getWithDefault(107) == Range<uint8_t>(0, 13) );
    }
    REQUIRE( synth.getRegionView(0)->randRange == Range<float>(0, 0.25) );
    REQUIRE( synth.getRegionView(1)->randRange == Range<float>(0.25, 0.5) );
    REQUIRE( synth.getRegionView(2)->randRange == Range<float>(0.5, 0.75) );
    REQUIRE( synth.getRegionView(3)->randRange == Range<float>(0.75, 1.0) );
    REQUIRE( synth.getRegionView(0)->sample == R"(../Samples/pizz/a0_vl4_rr1.wav)" );
    REQUIRE( synth.getRegionView(1)->sample == R"(../Samples/pizz/a0_vl4_rr2.wav)" );
    REQUIRE( synth.getRegionView(2)->sample == R"(../Samples/pizz/a0_vl4_rr3.wav)" );
    REQUIRE( synth.getRegionView(3)->sample == R"(../Samples/pizz/a0_vl4_rr4.wav)" );
}

// TEST_CASE("[Files] sw_default")
// {
//     const double sampleRate { 48000 };
//     const int blockSize { 256 };
//     sfz::Synth synth;
//     synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/sw_default.sfz");
//     REQUIRE( synth.getNumRegions() == 4 );
//     REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
// }

// TEST_CASE("[Files] sw_default and playing with switches")
// {
//     const double sampleRate { 48000 };
//     const int blockSize { 256 };
//     sfz::Synth synth;
//     synth.loadSfzFile(std::filesystem::current_path() / "tests/TestFiles/sw_default.sfz");
//     REQUIRE( synth.getNumRegions() == 4 );
//     REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
//     AudioBuffer<float> buffer { 2, blockSize };
//     synth.prepareToPlay(sampleRate, blockSize);
//     buffer.clear();
//     synth.registerNoteOn(1, 41, 64, 0);
//     REQUIRE( synth.getRegionView(0)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(1)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(2)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(3)->isSwitchedOn() );
//     synth.registerNoteOff(1, 41, 0, 0);
//     synth.registerNoteOn(1, 42, 64, 0);
//     REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(1)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(3)->isSwitchedOn() );
//     synth.registerNoteOff(1, 42, 0, 0);
//     synth.registerNoteOn(1, 40, 64, 0);
//     REQUIRE( !synth.getRegionView(0)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(1)->isSwitchedOn() );
//     REQUIRE( !synth.getRegionView(2)->isSwitchedOn() );
//     REQUIRE( synth.getRegionView(3)->isSwitchedOn() );
// }