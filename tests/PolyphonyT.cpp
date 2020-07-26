// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Synth.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"

using namespace Catch::literals;
using namespace sfz::literals;

constexpr int blockSize { 256 };


TEST_CASE("[Polyphony] Polyphony in hierarchy")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
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
    REQUIRE( synth.getRegionSetView(1)->getPolyphonyLimit() == 2 );
    REQUIRE( synth.getRegionView(1)->polyphony == 2 );
    REQUIRE( synth.getRegionSetView(2)->getPolyphonyLimit() == 3 );
    REQUIRE( synth.getRegionSetView(2)->getRegions()[0]->polyphony == 3 );
    REQUIRE( synth.getRegionSetView(3)->getPolyphonyLimit() == 4 );
    REQUIRE( synth.getRegionSetView(3)->getRegions()[0]->polyphony == 5 );
    REQUIRE( synth.getRegionSetView(3)->getRegions()[1]->polyphony == 4 );
}

TEST_CASE("[Polyphony] Polyphony groups")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
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
    synth.loadSfzString(fs::current_path(), R"(
        <group> group=1 polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2); // group polyphony should block the last note
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <group> polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (group)")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <group> polyphony=2
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (master)")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <master> polyphony=2
        <group> polyphony=5
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (limit in another master)")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <master> polyphony=2
        <region> sample=*saw key=65
        <master>
        <group> polyphony=5
        <region> sample=*sine key=66
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 66, 64);
    synth.noteOn(0, 66, 64);
    synth.noteOn(0, 66, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 5);
}

TEST_CASE("[Polyphony] Hierarchy polyphony limits (global)")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <global> polyphony=2
        <group> polyphony=5
        <region> sample=*sine key=65
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2);
}

TEST_CASE("[Polyphony] Polyphony in master")
{
    sfz::Synth synth;
    synth.setSamplesPerBlock(blockSize);
    sfz::AudioBuffer<float> buffer { 2, blockSize };
    synth.loadSfzString(fs::current_path(), R"(
        <master> polyphony=2
        <group> group=2
        <region> sample=*sine key=65
        <group> group=3
        <region> sample=*sine key=63
        <master> // Empty master resets the polyphony
        <region> sample=*sine key=61
    )");
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    synth.noteOn(0, 65, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2); // group polyphony should block the last note
    synth.allSoundOff();
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 63, 64);
    synth.noteOn(0, 63, 64);
    synth.noteOn(0, 63, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 2); // group polyphony should block the last note
    synth.allSoundOff();
    synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices(true) == 0);
    synth.noteOn(0, 61, 64);
    synth.noteOn(0, 61, 64);
    synth.noteOn(0, 61, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 3);
}


TEST_CASE("[Polyphony] Self-masking")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine key=64 note_polyphony=2
    )");
    synth.noteOn(0, 64, 63);
    synth.noteOn(0, 64, 62);
    synth.noteOn(0, 64, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 3); // One of these is releasing
    REQUIRE(synth.getVoiceView(0)->getTriggerValue() == 63_norm);
    REQUIRE(!synth.getVoiceView(0)->releasedOrFree());
    REQUIRE(synth.getVoiceView(1)->getTriggerValue() == 62_norm);
    REQUIRE(synth.getVoiceView(1)->releasedOrFree()); // The lowest velocity voice is the masking candidate
    REQUIRE(synth.getVoiceView(2)->getTriggerValue() == 64_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
}

TEST_CASE("[Polyphony] Not self-masking")
{
    sfz::Synth synth;
    synth.loadSfzString(fs::current_path(), R"(
        <region> sample=*sine key=66 note_polyphony=2 note_selfmask=off
    )");
    synth.noteOn(0, 66, 63);
    synth.noteOn(0, 66, 62);
    synth.noteOn(0, 66, 64);
    REQUIRE(synth.getNumActiveVoices(true) == 3); // One of these is releasing
    REQUIRE(synth.getVoiceView(0)->getTriggerValue() == 63_norm);
    REQUIRE(synth.getVoiceView(0)->releasedOrFree()); // The first encountered voice is the masking candidate
    REQUIRE(synth.getVoiceView(1)->getTriggerValue() == 62_norm);
    REQUIRE(!synth.getVoiceView(1)->releasedOrFree());
    REQUIRE(synth.getVoiceView(2)->getTriggerValue() == 64_norm);
    REQUIRE(!synth.getVoiceView(2)->releasedOrFree());
}
