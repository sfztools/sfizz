// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Synth.h"
#include "sfizz/SfzHelpers.h"
#include "TestHelpers.h"
#include <absl/algorithm/container.h>
#include "catch2/catch.hpp"

// Need these for the introspection of Synth
#include "sfizz/PolyphonyGroup.h"
#include "sfizz/RegionSet.h"

using namespace Catch::literals;
using namespace sfz::literals;

TEST_CASE("[Polyphony] Polyphony in hierarchy")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> key=61 sample=*sine polyphony=2
        <group> polyphony=2
        <region> key=62 sample=*sine
        <master> polyphony=3
        <region> key=63 sample=*sine
        <region> key=63 sample=*sine
        <region> key=63 sample=*sine
        <group> polyphony=4
        <region> key=64 sample=*sine polyphony=5
        <region> key=64 sample=*sine
        <region> key=64 sample=*sine
        <region> key=64 sample=*sine
    )");
    REQUIRE( synth.getRegionView(0)->polyphony == 2 );
    REQUIRE( synth.getRegionSetView(0)->getPolyphonyLimit() == 2 );
    REQUIRE( synth.getRegionView(1)->polyphony == 2 );
    REQUIRE( synth.getRegionSetView(1)->getPolyphonyLimit() == 3 );
    REQUIRE( synth.getRegionSetView(1)->getRegions()[0]->polyphony == 3 );
    REQUIRE( synth.getRegionSetView(2)->getPolyphonyLimit() == 4 );
    REQUIRE( synth.getRegionSetView(2)->getRegions()[0]->polyphony == 5 );
    REQUIRE( synth.getRegionSetView(2)->getRegions()[1]->polyphony == 4 );
}

TEST_CASE("[Polyphony] Polyphony groups")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> polyphony=2
        <region> key=62 sample=*sine
        <group> group=1 polyphony=3
        <region> key=63 sample=*sine
        <region> key=63 sample=*sine group=2 polyphony=4
        <region> key=63 sample=*sine group=4 polyphony=5
        <group> group=4
        <region> key=62 sample=*sine
    )");
    REQUIRE( synth.getNumPolyphonyGroups() == 5 );
    REQUIRE( synth.getNumRegions() == 5 );
    REQUIRE( synth.getRegionView(0)->group == 0 );
    REQUIRE( synth.getRegionView(1)->group == 1 );
    REQUIRE( synth.getRegionView(2)->group == 2 );
    REQUIRE( synth.getRegionView(3)->group == 4 );
    REQUIRE( synth.getRegionView(3)->polyphony == 5 );
    REQUIRE( synth.getRegionView(4)->group == 4 );
    REQUIRE( synth.getPolyphonyGroupView(1)->getPolyphonyLimit() == 3 );
    REQUIRE( synth.getPolyphonyGroupView(2)->getPolyphonyLimit() == 4 );
    REQUIRE( synth.getPolyphonyGroupView(3)->getPolyphonyLimit() == sfz::config::maxVoices );
    REQUIRE( synth.getPolyphonyGroupView(4)->getPolyphonyLimit() == 5 );
}

TEST_CASE("[Polyphony] group polyphony limits")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> group=1 polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (group)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (master)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <master> polyphony=2
        <group> polyphony=5
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (limit in another master)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <master> polyphony=2
        <region> sample=*saw key=65
        <master>
        <group> polyphony=5
        <region> sample=*sine key=66
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.noteOn(3, 66, 64);
    synth.noteOn(4, 66, 64);
    synth.noteOn(5, 66, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 6);
    REQUIRE( numPlayingVoices(synth) == 5); // One is releasing
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (global)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <global> polyphony=2
        <group> polyphony=5
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
}

TEST_CASE("[Polyphony] Polyphony in master")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <master> polyphony=2
        <group> group=2
        <region> sample=*sine key=65
        <group> group=3
        <region> sample=*sine key=63
        <master> // Empty master resets the polyphony
        <region> sample=*sine key=61
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(1, 65, 64);
    synth.noteOn(2, 65, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
    synth.allSoundOff();
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0);
    synth.noteOn(0, 63, 64);
    synth.noteOn(1, 63, 64);
    synth.noteOn(2, 63, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 2 ); // One is releasing
    synth.allSoundOff();
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 0);
    synth.noteOn(0, 61, 64);
    synth.noteOn(1, 61, 64);
    synth.noteOn(2, 61, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 );
    REQUIRE( numPlayingVoices(synth) == 3 );
}


TEST_CASE("[Polyphony] Self-masking")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> sample=*sine key=64 note_polyphony=2
    )");
    synth.noteOn(0, 64, 63 );
    synth.noteOn(1, 64, 62 );
    synth.noteOn(2, 64, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 ); // One of these is releasing
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 62_norm);
    REQUIRE( synth.getVoiceView(1)->releasedOrFree()); // The lowest velocity voice is the masking candidate
    REQUIRE( synth.getVoiceView(2)->getTriggerEvent().value == 64_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
}

TEST_CASE("[Polyphony] Not self-masking")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> sample=*sine key=66 note_polyphony=2 note_selfmask=off
    )");
    synth.noteOn(0, 66, 63 );
    synth.noteOn(1, 66, 62 );
    synth.noteOn(2, 66, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 ); // One of these is releasing
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE( synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 62_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
    REQUIRE( synth.getVoiceView(2)->getTriggerEvent().value == 64_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
}

TEST_CASE("[Polyphony] Self-masking with the exact same velocity")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine key=64 note_polyphony=2
    )");
    synth.noteOn(0, 64, 64);
    synth.noteOn(1, 64, 63 );
    synth.noteOn(2, 64, 63 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 3 ); // One of these is releasing
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 64_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 63_norm);
    REQUIRE( synth.getVoiceView(1)->releasedOrFree()); // The first one is the masking candidate since they have the same velocity
    REQUIRE( synth.getVoiceView(2)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
}

TEST_CASE("[Polyphony] Self-masking only works from low to high")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> sample=*sine key=64 note_polyphony=1
    )");
    synth.noteOn(0, 64, 63 );
    synth.noteOn(1, 64, 62 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 ); // Both notes are playing
    REQUIRE( numPlayingVoices(synth) == 2 ); // id
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 62_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony checks works across regions in the same polyphony group (default)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> sample=*saw key=64 note_polyphony=1
        <region> sample=*sine key=64 note_polyphony=1
    )");
    synth.noteOn(0, 64, 62 );
    synth.noteOn(1, 64, 63 );
    synth.renderBlock(buffer);
    REQUIRE( numPlayingVoices(synth) == 1 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 62_norm);
    REQUIRE( synth.getVoiceView(0)->releasedOrFree()); // got killed
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 62_norm);
    REQUIRE( synth.getVoiceView(1)->releasedOrFree()); // got killed
    REQUIRE( synth.getVoiceView(2)->getTriggerEvent().value == 63_norm);
    REQUIRE( synth.getVoiceView(2)->releasedOrFree()); // got killed
    REQUIRE( synth.getVoiceView(3)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(3)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony checks works across regions in the same polyphony group (default, with keyswitches)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <global> sw_lokey=36 sw_hikey=37 sw_default=36
        <region> sw_last=36 key=48 note_polyphony=1 sample=*saw
        <region> sw_last=37 key=48 transpose=12 note_polyphony=1 sample=*tri
    )");
    synth.noteOn(0, 48, 63 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1);
    synth.cc(1, 64, 127);
    synth.noteOn(2, 37, 127);
    synth.noteOff(3, 37, 0);
    synth.noteOn(4, 48, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE( numPlayingVoices(synth) == 1 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE( synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 64_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
}


TEST_CASE("[Polyphony] Note polyphony do not operate across polyphony groups")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> group=1 sample=*saw key=64 note_polyphony=1
        <region> group=2 sample=*sine key=64 note_polyphony=1
    )");
    synth.noteOn(0, 64, 62 );
    synth.noteOn(1, 64, 63 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 4); // Both notes are playing
    REQUIRE(numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 62_norm);
    REQUIRE( synth.getVoiceView(0)->releasedOrFree()); // got killed
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 62_norm);
    REQUIRE( synth.getVoiceView(1)->releasedOrFree()); // got killed
    REQUIRE( synth.getVoiceView(2)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
    REQUIRE( synth.getVoiceView(3)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(3)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony do not operate across polyphony groups (with keyswitches)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <global> sw_lokey=36 sw_hikey=37 sw_default=36
        <region> group=1 sw_last=36 key=48 note_polyphony=1 sample=*saw
        <region> group=2 sw_last=37 key=48 transpose=12 note_polyphony=1 sample=*tri
    )");
    synth.noteOn(0, 48, 63 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1);
    synth.cc(1, 64, 127);
    synth.noteOn(2, 37, 127);
    synth.noteOff(3, 37, 0);
    synth.noteOn(4, 48, 64);
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE(numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 64_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony operates on release voices")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> key=48 note_polyphony=1 sample=*saw trigger=release_key ampeg_attack=1 ampeg_decay=1
    )");
    synth.noteOn(0, 48, 63 );
    synth.noteOff(10, 48, 0 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1);
    synth.noteOn(20, 48, 65 );
    synth.noteOff(30, 48, 10 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE(numPlayingVoices(synth) == 1 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE( synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 65_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony operates on release voices (masking works from low to high but takes into account the replaced velocity)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> key=48 note_polyphony=1 sample=*saw trigger=release_key ampeg_attack=1 ampeg_decay=1
    )");
    synth.noteOn(0, 48, 63 );
    synth.noteOff(10, 48, 0 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 1);
    REQUIRE( numPlayingVoices(synth) == 1 );
    synth.noteOn(20, 48, 61 );
    synth.noteOff(30, 48, 10 );
    synth.renderBlock(buffer);
    REQUIRE( synth.getNumActiveVoices() == 2 );
    REQUIRE( numPlayingVoices(synth) == 2 );
    REQUIRE( synth.getVoiceView(0)->getTriggerEvent().value == 63_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE( synth.getVoiceView(1)->getTriggerEvent().value == 61_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
}

TEST_CASE("[Polyphony] Note polyphony operates on release voices and sustain pedal")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> key=48 sample=*sine
        <region> key=48 note_polyphony=1 sample=*saw trigger=release ampeg_attack=1 ampeg_decay=1
    )");
    synth.cc(0, 64, 127);
    synth.noteOn(0, 48, 61 );
    synth.noteOff(1, 48, 0 );
    synth.noteOn(2, 48, 62 );
    synth.noteOff(3, 48, 0 );
    synth.noteOn(4, 48, 63 );
    synth.noteOff(5, 48, 0 );
    synth.renderBlock(buffer);
    std::vector<std::string> expectedSamples { "*sine", "*sine", "*sine" };
    REQUIRE( playingSamples(synth) == expectedSamples );
    synth.cc(20, 64, 0);
    synth.renderBlock(buffer);
    std::vector<std::string> expectedSamples2 { "*saw" };
    std::vector<float> expectedVelocities { 63_norm };
    REQUIRE( playingSamples(synth) == expectedSamples2 );
    REQUIRE( playingVelocities(synth) == expectedVelocities );
}

TEST_CASE("[Polyphony] Note polyphony operates on release voices and sustain pedal (masking)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <region> key=48 sample=*sine
        <region> key=48 note_polyphony=1 sample=*saw trigger=release ampeg_attack=1 ampeg_decay=1
    )");
    synth.cc(0, 64, 127);
    synth.noteOn(0, 48, 63 );
    synth.noteOff(1, 48, 0 );
    synth.noteOn(2, 48, 62 );
    synth.noteOff(3, 48, 0 );
    synth.noteOn(4, 48, 61 );
    synth.noteOff(5, 48, 0 );
    synth.renderBlock(buffer);
    std::vector<std::string> expectedSamples { "*sine", "*sine", "*sine" };
    REQUIRE( playingSamples(synth) == expectedSamples );
    synth.cc(20, 64, 0);
    synth.renderBlock(buffer);
    std::vector<std::string> expectedSamples2 { "*saw", "*saw", "*saw" };
    std::vector<float> expectedVelocities { 63_norm, 62_norm, 61_norm };
    REQUIRE( playingSamples(synth) == expectedSamples2 );
    REQUIRE( playingVelocities(synth) == expectedVelocities );
}

TEST_CASE("[Polyphony] Bi-directional choking (with polyphony)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> key=60 polyphony=1
        <region> sample=kick.wav loop_mode=one_shot
        <region> sample=snare.wav trigger=release
    )");
    synth.noteOn(0, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "kick.wav" } );
    synth.noteOff(10, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "snare.wav" } );
    synth.noteOn(20, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "kick.wav" } );
}

TEST_CASE("[Polyphony] Bi-directional choking (with note_polyphony)")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/polyphony.sfz", R"(
        <group> key=60 note_polyphony=1
        <region> sample=kick.wav loop_mode=one_shot
        <region> sample=snare.wav trigger=release
    )");
    synth.noteOn(0, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "kick.wav" } );
    synth.noteOff(10, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "snare.wav" } );
    synth.noteOn(20, 60, 63 );
    synth.renderBlock(buffer);
    REQUIRE( playingSamples(synth) == std::vector<std::string> { "kick.wav" } );
}
