// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "TestHelpers.h"
#include "sfizz/Synth.h"
#include "catch2/catch.hpp"
#include <stdexcept>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>
#include "SynthDiscussion.h"

using namespace Catch::literals;
using namespace sfz;
using namespace sfz::literals;
using OSC = OSCValueLess;

TEST_CASE("Read values", "[parsing][OSC]")
{
    SynthDiscussion d;

    SECTION("Delay basic")
    {
        d.load(R"(
            <region> sample=*sine
            <region> sample=*sine delay=1
            <region> sample=*sine delay=-1
            <region> sample=*sine delay=1 delay=-1
        )");
        REQUIRE(d.read<float>("/region0/delay") == 0);
        REQUIRE(d.read<float>("/region1/delay") == 1);
        REQUIRE(d.read<float>("/region2/delay") == -1);
        REQUIRE(d.read<float>("/region3/delay") == -1);
    }

    SECTION("Delay random")
    {
        d.load(R"(
            <region> sample=*sine
            <region> sample=*sine delay_random=1
            <region> sample=*sine delay_random=-1
            <region> sample=*sine delay_random=1 delay_random=-1
        )");
        REQUIRE(d.read<float>("/region0/delay_random") == 0);
        REQUIRE(d.read<float>("/region1/delay_random") == 1);
        REQUIRE(d.read<float>("/region2/delay_random") == -1);
        REQUIRE(d.read<float>("/region3/delay_random") == -1);
    }

    SECTION("Delay CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav delay_cc12=1.5
            <region> sample=kick.wav delay_cc12=-1.5
            <region> sample=kick.wav delay_cc14=3 delay_cc12=2 delay_cc12=-12
        )");
        REQUIRE( d.read<float>("/region0/delay_cc12") == 0 );
        REQUIRE( d.read<float>("/region1/delay_cc12") == 1.5 );
        REQUIRE( d.read<float>("/region2/delay_cc12") == -1.5 );
        REQUIRE( d.read<float>("/region3/delay_cc14") == 3 );
        REQUIRE( d.read<float>("/region3/delay_cc12") == -12 );
    }

    SECTION("Sample and direction")
    {
        d.load(R"(
            <region> sample=*sine
            <region> sample=kick.wav
            <region> sample=kick.wav direction=reverse
        )");
        REQUIRE(d.read<std::string>("/region0/sample") == "*sine");
        REQUIRE(d.read<std::string>("/region1/sample") == "kick.wav");
        REQUIRE(d.read<std::string>("/region1/direction") == "forward");
        REQUIRE(d.read<std::string>("/region2/direction") == "reverse");
    }

    SECTION("Offset basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset=12
            <region> sample=kick.wav offset=-1
            <region> sample=kick.wav offset=12 offset=-1
        )");
        REQUIRE(d.read<int64_t>("/region0/offset") == 0);
        REQUIRE(d.read<int64_t>("/region1/offset") == 12);
        REQUIRE(d.read<int64_t>("/region2/offset") == -1);
        REQUIRE(d.read<int64_t>("/region3/offset") == -1);
    }

    SECTION("Offset random")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset_random=1
            <region> sample=kick.wav offset_random=-1
            <region> sample=kick.wav offset_random=1 offset_random=-1
        )");
        REQUIRE(d.read<int64_t>("/region0/offset_random") == 0);
        REQUIRE(d.read<int64_t>("/region1/offset_random") == 1);
        REQUIRE(d.read<int64_t>("/region2/offset_random") == -1);
        REQUIRE(d.read<int64_t>("/region3/offset_random") == -1);
    }

    SECTION("Offset CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset_cc12=12
            <region> sample=kick.wav offset_cc12=-12
            <region> sample=kick.wav offset_cc14=14 offset_cc12=12 offset_cc12=-12
        )");
        REQUIRE(d.read<int64_t>("/region0/offset_cc12") == 0);
        REQUIRE(d.read<int64_t>("/region1/offset_cc12") == 12);
        REQUIRE(d.read<int64_t>("/region2/offset_cc12") == -12);
        REQUIRE(d.read<int64_t>("/region3/offset_cc14") == 14);
        REQUIRE(d.read<int64_t>("/region3/offset_cc12") == -12);
    }

    SECTION("Sample end basic")
    {
        d.load(R"(
            <region> sample=kick.wav end=194
            <region> sample=kick.wav end=-1
            <region> sample=kick.wav end=0
            <region> sample=kick.wav end=194 end=-1
            <region> sample=kick.wav end=0 end=194
        )");
        REQUIRE(d.read<int64_t>("/region0/end") == 194);
        REQUIRE(d.read<OSC>("/region0/enabled") == OSC::True);
        REQUIRE(d.read<OSC>("/region1/enabled") == OSC::False);
        REQUIRE(d.read<OSC>("/region2/enabled") == OSC::False);
        REQUIRE(d.read<OSC>("/region3/enabled") == OSC::False);
        REQUIRE(d.read<OSC>("/region4/enabled") == OSC::True);
        REQUIRE(d.read<int64_t>("/region4/end") == 194);
    }

    SECTION("Sample end CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav end_cc12=12
            <region> sample=kick.wav end_oncc12=-12
            <region> sample=kick.wav end_cc14=14 end_cc12=12 end_oncc12=-12
        )");
        REQUIRE(d.read<int64_t>("/region0/end_cc12") == 0);
        REQUIRE(d.read<int64_t>("/region1/end_cc12") == 12);
        REQUIRE(d.read<int64_t>("/region2/end_cc12") == -12);
        REQUIRE(d.read<int64_t>("/region3/end_cc14") == 14);
        REQUIRE(d.read<int64_t>("/region3/end_cc12") == -12);
    }
    SECTION("Count") 
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav count=2
            <region> sample=kick.wav count=-1
        )");
        REQUIRE(d.read<OSC>("/region0/count") == OSC::None);
        REQUIRE(d.read<int32_t>("/region1/count") == 2);
        REQUIRE(d.read<OSC>("/region2/count") == OSC::None);
    }

    SECTION("Loop mode")
    {
        SynthDiscussion d;
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_mode=one_shot
            <region> sample=kick.wav loopmode=one_shot
            <region> sample=kick.wav loop_mode=loop_sustain
            <region> sample=kick.wav loop_mode=loop_continuous
            <region> sample=kick.wav loop_mode=loop_continuous loop_mode=no_loop
        )");
        REQUIRE(d.read<std::string>("/region0/loop_mode") == "no_loop");
        REQUIRE(d.read<std::string>("/region1/loop_mode") == "one_shot");
        REQUIRE(d.read<std::string>("/region2/loop_mode") == "one_shot");
        REQUIRE(d.read<std::string>("/region3/loop_mode") == "loop_sustain");
        REQUIRE(d.read<std::string>("/region4/loop_mode") == "loop_continuous");
        REQUIRE(d.read<std::string>("/region5/loop_mode") == "no_loop");
    }

    SECTION("Loop range basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_start=10 loop_end=100
            <region> sample=kick.wav loopstart=10 loopend=100
            <region> sample=kick.wav loop_start=-1 loopend=-100
        )");
        REQUIRE_THAT(d.readAll<int64_t>("/region0/loop_range"), Catch::Approx(std::vector<int64_t>{ 0, 44011 }));
        REQUIRE_THAT(d.readAll<int64_t>("/region1/loop_range"), Catch::Approx(std::vector<int64_t>{ 10, 100 }));
        REQUIRE_THAT(d.readAll<int64_t>("/region2/loop_range"), Catch::Approx(std::vector<int64_t>{ 10, 100 }));
        REQUIRE_THAT(d.readAll<int64_t>("/region3/loop_range"), Catch::Approx(std::vector<int64_t>{ 0, 44011 }));
    }

    SECTION("Loop range CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_start_cc12=10 loop_end_cc14=-100
            <region> sample=kick.wav loop_start_oncc12=-10 loop_end_oncc14=100
            <region> sample=kick.wav loop_startcc12=-10 loop_lengthcc14=100
            <region> sample=kick.wav loop_length_oncc14=100
            <region> sample=kick.wav loop_length_cc14=100
        )");
        REQUIRE(d.read<int64_t>("/region0/loop_start_cc12") == 0);
        REQUIRE(d.read<int64_t>("/region0/loop_end_cc14") == 0);
        REQUIRE(d.read<int64_t>("/region1/loop_start_cc12") == 10);
        REQUIRE(d.read<int64_t>("/region1/loop_end_cc14") == -100);
        REQUIRE(d.read<int64_t>("/region2/loop_start_cc12") == -10);
        REQUIRE(d.read<int64_t>("/region2/loop_end_cc14") == 100);
        REQUIRE(d.read<int64_t>("/region3/loop_start_cc12") == -10);
        REQUIRE(d.read<int64_t>("/region3/loop_end_cc14") == 100);
        REQUIRE(d.read<int64_t>("/region4/loop_end_cc14") == 100);
        REQUIRE(d.read<int64_t>("/region5/loop_end_cc14") == 100);
    }

    SECTION("Loop crossfade")
    {
        d.load(R"(
            <region> sample=kick.wav loop_crossfade=0.5
            <region> sample=kick.wav loop_crossfade=-1
        )");
        REQUIRE(d.read<float>("/region0/loop_crossfade") == 0.5f);
        REQUIRE(d.read<float>("/region1/loop_crossfade") == 0.001f);
    }

    SECTION("Loop count")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_count=2
            <region> sample=kick.wav loop_count=-1
        )");
        REQUIRE(d.read<OSC>("/region0/loop_count") == OSC::None);
        REQUIRE(d.read<int32_t>("/region1/loop_count") == 2);
        REQUIRE(d.read<OSC>("/region2/loop_count") == OSC::None);
    }

    SECTION("No special outputs") {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE(d.read<int32_t>("/region0/output") == 0);
        REQUIRE(d.read<int32_t>("/num_outputs") == 1);
    }

    SECTION("1 output") {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav output=1
            <region> sample=kick.wav output=-1
        )");
        REQUIRE(d.read<int32_t>("/region0/output") == 0);
        REQUIRE(d.read<int32_t>("/region1/output") == 1);
        REQUIRE(d.read<int32_t>("/region2/output") == 0);
        REQUIRE(d.read<int32_t>("/num_outputs") == 2);
    }

    SECTION("More than 1 output") {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav output=1
            <region> sample=kick.wav output=3
        )");
        REQUIRE(d.read<int32_t>("/region0/output") == 0);
        REQUIRE(d.read<int32_t>("/region1/output") == 1);
        REQUIRE(d.read<int32_t>("/region2/output") == 3);
        REQUIRE(d.read<int32_t>("/num_outputs") == 4);
    }

    SECTION("Group")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav group=5
            <region> sample=kick.wav group=-2
        )");
        REQUIRE(d.read<int64_t>("/region0/group") == 0);
        REQUIRE(d.read<int64_t>("/region1/group") == 5);
        REQUIRE(d.read<int64_t>("/region2/group") == -2);
    }

    SECTION("Off by")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav off_by=5
            <region> sample=kick.wav off_by=-2
        )");
        REQUIRE( d.read<OSC>("/region0/off_by") == OSC::None );
        REQUIRE( d.read<int64_t>("/region1/off_by") == 5 );
        REQUIRE( d.read<int64_t>("/region2/off_by") == -2 );
    }

    SECTION("Off mode")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav off_mode=fast
            <region> sample=kick.wav off_mode=normal
            <region> sample=kick.wav off_mode=time
            <region> sample=kick.wav off_mode=time off_mode=normal
            <region> sample=kick.wav off_mode=nothing
        )");
        REQUIRE( d.read<std::string>("/region0/off_mode") == "fast" );
        REQUIRE( d.read<std::string>("/region1/off_mode") == "fast" );
        REQUIRE( d.read<std::string>("/region2/off_mode") == "normal" );
        REQUIRE( d.read<std::string>("/region3/off_mode") == "time" );
        REQUIRE( d.read<std::string>("/region4/off_mode") == "normal" );
        REQUIRE( d.read<std::string>("/region5/off_mode") == "fast" );
    }

    SECTION("Off time")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav off_time=0.1
            <region> sample=kick.wav off_time=-1
        )");
        REQUIRE( d.read<float>("/region0/off_time") == 0.006f);
        REQUIRE( d.read<float>("/region1/off_time") == 0.1f);
        REQUIRE( d.read<float>("/region2/off_time") == -1.0f);
    }

    SECTION("Key range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lokey=34 hikey=60
            <region> sample=kick.wav lokey=c4 hikey=b5
            <region> sample=kick.wav lokey=-3 hikey=60
            <region> sample=kick.wav hikey=-1
            <region> sample=kick.wav pitch_keycenter=32
            <region> sample=kick.wav pitch_keycenter=-1
            <region> sample=kick.wav key=26
        )");
        REQUIRE_THAT( d.readAll<int32_t>("/region0/key_range"), Catch::Approx(std::vector<int32_t>{ 0, 127 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region1/key_range"), Catch::Approx(std::vector<int32_t>{ 34, 60 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region2/key_range"), Catch::Approx(std::vector<int32_t>{ 60, 83 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region3/key_range"), Catch::Approx(std::vector<int32_t>{ 0, 60 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region4/key_range"), Catch::Approx(std::vector<int32_t>{ 0, 127 }));
        REQUIRE( d.read<int32_t>("/region0/pitch_keycenter") == 60);
        REQUIRE( d.read<int32_t>("/region5/pitch_keycenter") == 32);
        REQUIRE( d.read<int32_t>("/region6/pitch_keycenter") == 60);
        REQUIRE_THAT( d.readAll<int32_t>("/region7/key_range"), Catch::Approx(std::vector<int32_t>{ 26, 26 }));
        REQUIRE( d.read<int32_t>("/region7/pitch_keycenter") == 26);
    }

    SECTION("Triggers on note")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav hikey=-1
            <region> sample=kick.wav key=-1
            <region> sample=kick.wav hikey=-1 lokey=12
            <region> sample=kick.wav hikey=-1 lokey=-1
            <region> sample=kick.wav hikey=0 lokey=12
        )");
        REQUIRE( d.read<OSC>("/region0/trigger_on_note") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/trigger_on_note") == OSC::False);
        REQUIRE( d.read<OSC>("/region2/trigger_on_note") == OSC::False);
        // TODO: Double check with Sforzando/rgc
        REQUIRE( d.read<OSC>("/region3/trigger_on_note") == OSC::False);
        REQUIRE( d.read<OSC>("/region4/trigger_on_note") == OSC::False);
        REQUIRE( d.read<OSC>("/region5/trigger_on_note") == OSC::True);
    }

    SECTION("Velocity range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lovel=34 hivel=60
            <region> sample=kick.wav lovel=-3 hivel=60
            <region> sample=kick.wav hivel=-1
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/vel_range"), Catch::Approx(std::vector<float>{ 0.0f, 1.0f }));
        REQUIRE_THAT( d.readAll<float>("/region1/vel_range"), Catch::Approx(std::vector<float>{ 34_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/vel_range"), Catch::Approx(std::vector<float>{ -3_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/vel_range"), Catch::Approx(std::vector<float>{ 0.0f, -1_norm }));
    }

    SECTION("Bend range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lobend=891 hibend=2000
            <region> sample=kick.wav lobend=-891 hibend=891
            <region> sample=kick.wav hibend=-10000
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/bend_range"), Catch::Approx(std::vector<float>{ -1.0f, 1.0f }));
        REQUIRE_THAT( d.readAll<float>("/region1/bend_range"), Catch::Approx(std::vector<float>{ 891.0_bend, 2000.0_bend }));
        REQUIRE_THAT( d.readAll<float>("/region2/bend_range"), Catch::Approx(std::vector<float>{ -891.0_bend, 891.0_bend }));
        REQUIRE_THAT( d.readAll<float>("/region3/bend_range"), Catch::Approx(std::vector<float>{ -1.0f, -10000.0_bend }));
    }

    SECTION("Program range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loprog=1 hiprog=45
            <region> sample=kick.wav loprog=-1 hiprog=555
            <region> sample=kick.wav hiprog=-1
        )");
        REQUIRE_THAT( d.readAll<int32_t>("/region0/program_range"), Catch::Approx(std::vector<int32_t>{ 0, 127 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region1/program_range"), Catch::Approx(std::vector<int32_t>{ 1, 45 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region2/program_range"), Catch::Approx(std::vector<int32_t>{ 0, 127 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region3/program_range"), Catch::Approx(std::vector<int32_t>{ 0, 127 }));
    }

    SECTION("CC condition basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav locc1=0 hicc1=54
            <region> sample=kick.wav locc1=0 hicc1=54 locc2=2 hicc2=10
            <region> sample=kick.wav locc1=10 hicc1=-1
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 1.0f }));
        REQUIRE_THAT( d.readAll<float>("/region1/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 55_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 55_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range2"), Catch::Approx(std::vector<float>{ 2_norm, 11_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/cc_range1"), Catch::Approx(std::vector<float>{ 10_norm, -1_norm }));
    }

    SECTION("HDCC conditions")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lohdcc1=0 hihdcc1=0.1
            <region> sample=kick.wav lohdcc1=0 hihdcc1=0.1 lohdcc2=0.1 hihdcc2=0.2
            <region> sample=kick.wav lohdcc1=0.1 hihdcc1=-0.1
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 1.0f }));
        REQUIRE_THAT( d.readAll<float>("/region1/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 0.1f }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 0.1f }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range2"), Catch::Approx(std::vector<float>{ 0.1f, 0.2f }));
        REQUIRE_THAT( d.readAll<float>("/region3/cc_range1"), Catch::Approx(std::vector<float>{ 0.1f, -0.1f }));
    }

    SECTION("RealCC conditions ")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lorealcc1=0 hirealcc1=0.1
            <region> sample=kick.wav lorealcc1=0 hirealcc1=0.1 lorealcc2=0.1 hirealcc2=0.2
            <region> sample=kick.wav lorealcc1=0.1 hirealcc1=-0.1
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 1.0f }));
        REQUIRE_THAT( d.readAll<float>("/region1/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 0.1f }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range1"), Catch::Approx(std::vector<float>{ 0.0f, 0.1f }));
        REQUIRE_THAT( d.readAll<float>("/region2/cc_range2"), Catch::Approx(std::vector<float>{ 0.1f, 0.2f }));
        REQUIRE_THAT( d.readAll<float>("/region3/cc_range1"), Catch::Approx(std::vector<float>{ 0.1f, -0.1f }));
    }

    SECTION("Last keyswitch basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_last=12
            <region> sample=kick.wav sw_last=c4
            <region> sample=kick.wav sw_lolast=14 sw_hilast=16
            <region> sample=kick.wav sw_lolast=c4 sw_hilast=b5
            <region> sample=kick.wav sw_last=-1
        )");
        REQUIRE( d.read<OSC>("/region0/sw_last") == OSC::None);
        REQUIRE( d.read<int32_t>("/region1/sw_last") == 12);
        REQUIRE( d.read<int32_t>("/region2/sw_last") == 60);
        REQUIRE_THAT( d.readAll<int32_t>("/region3/sw_last"), Catch::Approx(std::vector<int32_t>{14, 16} ));
        // TODO: activate for the new region parser ; can handle note names
        // REQUIRE( d.readAll<int32_t>("/region4/sw_last") == );
        // TODO: activate for the new region parser ; ignore the second value
        // REQUIRE( d.readAll<int32_t>("/region5/sw_last") == );
    }

    SECTION("sw_lolast disables sw_last over the whole region")
    {
        d.load(R"(
            <region> sample=kick.wav sw_last=12 sw_lolast=14 sw_last=16
        )");
        REQUIRE_THAT( d.readAll<int32_t>("/region0/sw_last"), Catch::Approx(std::vector<int32_t>{14, 14} ));
        std::vector<std::string> expected {
            "/region0/sw_last,ii : { 14, 14 }",
        };
    }

    SECTION("Keyswitch label")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_label=hello
        )");
        REQUIRE( d.read<OSC>("/region0/sw_label") == OSC::None );
        REQUIRE( d.read<std::string>("/region1/sw_label") == "hello" );
    }

    SECTION("Upswitch")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_up=16
            <region> sample=kick.wav sw_up=-1
            <region> sample=kick.wav sw_up=128
            <region> sample=kick.wav sw_up=c4
            <region> sample=kick.wav sw_up=64 sw_up=-1
        )");
        REQUIRE( d.read<OSC>("/region0/sw_up") == OSC::None);
        REQUIRE( d.read<int32_t>("/region1/sw_up") == 16);
        REQUIRE( d.read<OSC>("/region2/sw_up") == OSC::None);
        REQUIRE( d.read<OSC>("/region3/sw_up") == OSC::None);
        REQUIRE( d.read<int32_t>("/region4/sw_up") == 60);
        REQUIRE( d.read<OSC>("/region5/sw_up") == OSC::None);
    }

    SECTION("Downswitch")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_down=16
            <region> sample=kick.wav sw_down=-1
            <region> sample=kick.wav sw_down=128
            <region> sample=kick.wav sw_down=c4
            <region> sample=kick.wav sw_down=64 sw_down=-1
        )");
        REQUIRE( d.read<OSC>("/region0/sw_down") == OSC::None);
        REQUIRE( d.read<int32_t>("/region1/sw_down") == 16);
        REQUIRE( d.read<OSC>("/region2/sw_down") == OSC::None);
        REQUIRE( d.read<OSC>("/region3/sw_down") == OSC::None);
        REQUIRE( d.read<int32_t>("/region4/sw_down") == 60);
        REQUIRE( d.read<OSC>("/region5/sw_down") == OSC::None);
    }

    SECTION("Previous keyswitch")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_previous=16
            <region> sample=kick.wav sw_previous=-1
            <region> sample=kick.wav sw_previous=128
            <region> sample=kick.wav sw_previous=c4
            <region> sample=kick.wav sw_previous=64 sw_previous=-1
        )");
        REQUIRE( d.read<OSC>("/region0/sw_previous") == OSC::None);
        REQUIRE( d.read<int32_t>("/region1/sw_previous") == 16);
        REQUIRE( d.read<OSC>("/region2/sw_previous") == OSC::None);
        REQUIRE( d.read<OSC>("/region3/sw_previous") == OSC::None);
        REQUIRE( d.read<int32_t>("/region4/sw_previous") == 60);
        REQUIRE( d.read<OSC>("/region5/sw_previous") == OSC::None);
    }

    SECTION("Velocity override")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_vel=current
            <region> sample=kick.wav sw_vel=previous
            <region> sample=kick.wav sw_vel=previous sw_vel=current
        )");
        REQUIRE( d.read<std::string>("/region0/sw_vel") == "current");
        REQUIRE( d.read<std::string>("/region1/sw_vel") == "current");
        REQUIRE( d.read<std::string>("/region2/sw_vel") == "previous");
        REQUIRE( d.read<std::string>("/region3/sw_vel") == "current");
    }

    SECTION("Aftertouch range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lochanaft=34 hichanaft=60
            <region> sample=kick.wav lochanaft=-3 hichanaft=60
            <region> sample=kick.wav lochanaft=20 hichanaft=-1
            <region> sample=kick.wav lochanaft=20 hichanaft=10
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/chanaft_range"), Catch::Approx(std::vector<float>{ 0, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region1/chanaft_range"), Catch::Approx(std::vector<float>{ 34_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/chanaft_range"), Catch::Approx(std::vector<float>{ -3_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/chanaft_range"), Catch::Approx(std::vector<float>{ 20_norm, -1_norm }));
        REQUIRE_THAT( d.readAll<float>("/region4/chanaft_range"), Catch::Approx(std::vector<float>{ 20_norm, 11_norm }));
    }

    SECTION("Polyaftertouch range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lopolyaft=34 hipolyaft=60
            <region> sample=kick.wav lopolyaft=-3 hipolyaft=60
            <region> sample=kick.wav lopolyaft=20 hipolyaft=-1
            <region> sample=kick.wav lopolyaft=20 hipolyaft=10
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/polyaft_range"), Catch::Approx(std::vector<float>{ 0, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region1/polyaft_range"), Catch::Approx(std::vector<float>{ 34_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/polyaft_range"), Catch::Approx(std::vector<float>{ -3_norm, 61_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/polyaft_range"), Catch::Approx(std::vector<float>{ 20_norm, -1_norm }));
        REQUIRE_THAT( d.readAll<float>("/region4/polyaft_range"), Catch::Approx(std::vector<float>{ 20_norm, 11_norm }));
    }

    SECTION("BPM range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lobpm=34.1 hibpm=60.2
            <region> sample=kick.wav lobpm=-3 hibpm=60
            <region> sample=kick.wav lobpm=20 hibpm=-1
            <region> sample=kick.wav lobpm=20 hibpm=10
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/bpm_range"), Catch::Approx(std::vector<float>{ 0, 500 }));
        REQUIRE_THAT( d.readAll<float>("/region1/bpm_range"), Catch::Approx(std::vector<float>{ 34.1, 60.2 }));
        REQUIRE_THAT( d.readAll<float>("/region2/bpm_range"), Catch::Approx(std::vector<float>{ -3, 60 }));
        REQUIRE_THAT( d.readAll<float>("/region3/bpm_range"), Catch::Approx(std::vector<float>{ 20, -1 }));
        REQUIRE_THAT( d.readAll<float>("/region4/bpm_range"), Catch::Approx(std::vector<float>{ 20, 10 }));
    }

    SECTION("Rand range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lorand=0.2 hirand=0.4
            <region> sample=kick.wav lorand=-0.1 hirand=0.4
            <region> sample=kick.wav lorand=0.2 hirand=-0.1
            <region> sample=kick.wav lorand=0.2 hirand=0.1
        )");

        REQUIRE_THAT( d.readAll<float>("/region0/rand_range"), Catch::Approx(std::vector<float>{ 0, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region1/rand_range"), Catch::Approx(std::vector<float>{ 0.2, 0.4 }));
        REQUIRE_THAT( d.readAll<float>("/region2/rand_range"), Catch::Approx(std::vector<float>{ -0.1, 0.4 }));
        REQUIRE_THAT( d.readAll<float>("/region3/rand_range"), Catch::Approx(std::vector<float>{ 0.2, -0.1 }));
        REQUIRE_THAT( d.readAll<float>("/region4/rand_range"), Catch::Approx(std::vector<float>{ 0.2, 0.1 }));
    }

    SECTION("Timer range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lotimer=0.2 hitimer=0.4
            <region> sample=kick.wav lotimer=-0.1 hitimer=0.4
            <region> sample=kick.wav lotimer=0.2 hitimer=-0.1
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/timer_range"), Catch::Approx(std::vector<float>{ 0, 3.40282e+38 }));
        REQUIRE_THAT( d.readAll<float>("/region1/timer_range"), Catch::Approx(std::vector<float>{ 0.2, 0.4 }));
        REQUIRE_THAT( d.readAll<float>("/region2/timer_range"), Catch::Approx(std::vector<float>{ 0, 0.4 }));
        REQUIRE_THAT( d.readAll<float>("/region3/timer_range"), Catch::Approx(std::vector<float>{ 0.2, 3.40282e+38 }));
        REQUIRE( d.read<OSC>("/region0/use_timer_range") == OSC::False);
        REQUIRE( d.read<OSC>("/region1/use_timer_range") == OSC::True);
        REQUIRE( d.read<OSC>("/region2/use_timer_range") == OSC::True);
        REQUIRE( d.read<OSC>("/region3/use_timer_range") == OSC::True);
    }

    SECTION("Sequence length")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav seq_length=12
            <region> sample=kick.wav seq_length=-1
            <region> sample=kick.wav seq_length=12 seq_length=-1
        )");
        REQUIRE( d.read<int32_t>("/region0/seq_length") == 1);
        REQUIRE( d.read<int32_t>("/region1/seq_length") == 12);
        REQUIRE( d.read<int32_t>("/region2/seq_length") == 1);
        // TODO: activate for the new region parser ; ignore the second value
        // REQUIRE( d.read<int32_t>("/region3/seq_length") == 12);
    }

    SECTION("Sequence position")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav seq_position=12
            <region> sample=kick.wav seq_position=-1
            <region> sample=kick.wav seq_position=12 seq_position=-1
        )");
        REQUIRE( d.read<int32_t>("/region0/seq_position") == 1);
        REQUIRE( d.read<int32_t>("/region1/seq_position") == 12);
        REQUIRE( d.read<int32_t>("/region2/seq_position") == 1);
        // TODO: activate for the new region parser ; ignore the second value
        // REQUIRE( d.read<int32_t>("/region3/seq_position") == );
    }

    SECTION("Trigger type")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav trigger=release
            <region> sample=kick.wav trigger=release_key
            <region> sample=kick.wav trigger=legato
            <region> sample=kick.wav trigger=first
            <region> sample=kick.wav trigger=nothing
            <region> sample=kick.wav trigger=release trigger=attack
        )");
        REQUIRE( d.read<std::string>("/region0/trigger") == "attack");
        REQUIRE( d.read<std::string>("/region1/trigger") == "release");
        REQUIRE( d.read<std::string>("/region2/trigger") == "release_key");
        REQUIRE( d.read<std::string>("/region3/trigger") == "legato");
        REQUIRE( d.read<std::string>("/region4/trigger") == "first");
        REQUIRE( d.read<std::string>("/region5/trigger") == "attack");
        REQUIRE( d.read<std::string>("/region6/trigger") == "attack");
    }

    SECTION("Start on cc range")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav on_locc1=15
            <region> sample=kick.wav on_hicc1=84
            <region> sample=kick.wav on_locc1=15 on_hicc1=84
            <region> sample=kick.wav on_lohdcc2=0.1
            <region> sample=kick.wav on_hihdcc2=0.4
            <region> sample=kick.wav on_lohdcc2=0.1 on_hihdcc2=0.4
        )");
        REQUIRE( d.read<OSC>("/region0/start_cc_range1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/start_cc_range2") == OSC::None);
        REQUIRE_THAT( d.readAll<float>("/region1/start_cc_range1"), Catch::Approx(std::vector<float>{ 15_norm, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region2/start_cc_range1"), Catch::Approx(std::vector<float>{ 0, 85_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/start_cc_range1"), Catch::Approx(std::vector<float>{ 15_norm, 85_norm }));
        REQUIRE_THAT( d.readAll<float>("/region4/start_cc_range2"), Catch::Approx(std::vector<float>{ 0.1, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region5/start_cc_range2"), Catch::Approx(std::vector<float>{ 0, 0.4f }));
        REQUIRE_THAT( d.readAll<float>("/region6/start_cc_range2"), Catch::Approx(std::vector<float>{ 0.1, 0.4f }));
    }

    SECTION("Volume Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume=4.2
            <region> sample=kick.wav gain=-200
        )");
        REQUIRE( d.read<float>("/region0/volume") == 0);
        REQUIRE( d.read<float>("/region1/volume") == 4.2f);
        // TODO: activate for the new region parser ; allow oob
        // REQUIRE( d.read<>("/region2/volume") == );
    }

    SECTION("Volume CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume_oncc42=4.2
            <region> sample=kick.wav gain_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/volume_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/volume_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/volume_cc2") == -10.0f);
    }

    SECTION("Volume CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume_stepcc42=4.2
            <region> sample=kick.wav volume_smoothcc42=4
            <region> sample=kick.wav volume_curvecc42=2
            <region> sample=kick.wav volume_stepcc42=-1
            <region> sample=kick.wav volume_smoothcc42=-4
            <region> sample=kick.wav volume_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/volume_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/volume_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/volume_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/volume_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/volume_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/volume_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/volume_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/volume_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/volume_curvecc42") == OSC::None);
    }

    SECTION("Volume CC Params (with gain_)")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav gain_stepcc42=4.2
            <region> sample=kick.wav gain_smoothcc42=4
            <region> sample=kick.wav gain_curvecc42=2
            <region> sample=kick.wav gain_stepcc42=-1
            <region> sample=kick.wav gain_smoothcc42=-4
            <region> sample=kick.wav gain_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/volume_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/volume_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/volume_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/volume_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/volume_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/volume_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/volume_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/volume_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/volume_curvecc42") == OSC::None);
    }

    SECTION("Pan Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan=4.2
            <region> sample=kick.wav pan=-200
        )");
        REQUIRE( d.read<float>("/region0/pan") == 0);
        REQUIRE( d.read<float>("/region1/pan") == 4.2f);
        // TODO: activate for the new region parser ; allow oob
        // REQUIRE( d.read<>("/region2/pan") == );
    }

    SECTION("Pan CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan_oncc42=4.2
            <region> sample=kick.wav pan_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/pan_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/pan_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/pan_cc2") == -10.0f);
    }

    SECTION("Pan CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan_stepcc42=4.2
            <region> sample=kick.wav pan_smoothcc42=4
            <region> sample=kick.wav pan_curvecc42=2
            <region> sample=kick.wav pan_stepcc42=-1
            <region> sample=kick.wav pan_smoothcc42=-4
            <region> sample=kick.wav pan_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/pan_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pan_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pan_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/pan_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/pan_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/pan_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/pan_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/pan_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/pan_curvecc42") == OSC::None);
    }

    SECTION("Width Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width=4.2
            <region> sample=kick.wav width=-200
        )");
        REQUIRE( d.read<float>("/region0/width") == 100.0f);
        REQUIRE( d.read<float>("/region1/width") == 4.2f);
        // TODO: activate for the new region parser ; allow oob
        // REQUIRE( d.read<>("/region2/width") == -200.0f);
    }

    SECTION("Width CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width_oncc42=4.2
            <region> sample=kick.wav width_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/width_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/width_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/width_cc2") == -10.0f);
    }

    SECTION("Width CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width_stepcc42=4.2
            <region> sample=kick.wav width_smoothcc42=4
            <region> sample=kick.wav width_curvecc42=2
            <region> sample=kick.wav width_stepcc42=-1
            <region> sample=kick.wav width_smoothcc42=-4
            <region> sample=kick.wav width_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/width_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/width_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/width_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/width_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/width_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/width_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/width_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/width_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/width_curvecc42") == OSC::None);
    }

    SECTION("Position Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position=4.2
            <region> sample=kick.wav position=-200
        )");
        REQUIRE( d.read<float>("/region0/position") == 0);
        REQUIRE( d.read<float>("/region1/position") == 4.2f);
        // TODO: activate for the new region parser ; allow oob
        // REQUIRE( d.read<>("/region2/position") == );
    }

    SECTION("Position CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position_oncc42=4.2
            <region> sample=kick.wav position_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/position_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/position_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/position_cc2") == -10.0f);
    }

    SECTION("Position CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position_stepcc42=4.2
            <region> sample=kick.wav position_smoothcc42=4
            <region> sample=kick.wav position_curvecc42=2
            <region> sample=kick.wav position_stepcc42=-1
            <region> sample=kick.wav position_smoothcc42=-4
            <region> sample=kick.wav position_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/position_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/position_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/position_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/position_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/position_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/position_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/position_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/position_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/position_curvecc42") == OSC::None);
    }

    SECTION("Amplitude Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude=4.2
            <region> sample=kick.wav amplitude=-200
        )");
        REQUIRE( d.read<float>("/region0/amplitude") == 100.0f);
        REQUIRE( d.read<float>("/region1/amplitude") == 4.2f);
        // TODO: activate for the new region parser ; allow oob
        // REQUIRE( d.read<>("/region2/amplitude") == );
    }

    SECTION("Amplitude CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude_oncc42=4.2
            <region> sample=kick.wav amplitude_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/amplitude_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/amplitude_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/amplitude_cc2") == -10.0f);
    }

    SECTION("Amplitude CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude_stepcc42=4.2
            <region> sample=kick.wav amplitude_smoothcc42=4
            <region> sample=kick.wav amplitude_curvecc42=2
            <region> sample=kick.wav amplitude_stepcc42=-1
            <region> sample=kick.wav amplitude_smoothcc42=-4
            <region> sample=kick.wav amplitude_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/amplitude_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/amplitude_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/amplitude_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/amplitude_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/amplitude_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/amplitude_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/amplitude_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/amplitude_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/amplitude_curvecc42") == OSC::None);
    }

    SECTION("Amp Keycenter")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amp_keycenter=40
            <region> sample=kick.wav amp_keycenter=-1
            <region> sample=kick.wav amp_keycenter=c3
        )");
        REQUIRE( d.read<int32_t>("/region0/amp_keycenter") == 60 );
        REQUIRE( d.read<int32_t>("/region1/amp_keycenter") == 40 );
        REQUIRE( d.read<int32_t>("/region2/amp_keycenter") == 60 );
        REQUIRE( d.read<int32_t>("/region3/amp_keycenter") == 48 );
    }

    SECTION("Amp Keytrack")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amp_keytrack=10.1
            <region> sample=kick.wav amp_keytrack=40
        )");
        REQUIRE( d.read<float>("/region0/amp_keytrack") ==  0.0f);
        REQUIRE( d.read<float>("/region1/amp_keytrack") ==  10.1f);
        REQUIRE( d.read<float>("/region2/amp_keytrack") ==  40.0f);
        std::vector<std::string> expected {
            "/region0/amp_keytrack,f : {}",
            "/region1/amp_keytrack,f : {}",
            // "/region2/amp_keytrack,f : {}",
        };
    }

SECTION("Amp Veltrack")
{
    SECTION("Amp veltrack basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amp_veltrack=10.1
            <region> sample=kick.wav amp_veltrack=-132
        )");
        REQUIRE( d.read<float>("/region0/amp_veltrack") == 100.0f);
        REQUIRE( d.read<float>("/region1/amp_veltrack") == 10.1f);
        REQUIRE( d.read<float>("/region2/amp_veltrack") == -132.0f);
    }

    SECTION("Amp veltrack CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amp_veltrack_cc1=10.1 amp_veltrack_curvecc1=3
            <region> sample=kick.wav amp_veltrack_oncc2=-40 amp_veltrack_curvecc3=4
        )");
        REQUIRE( d.read<OSC>("/region0/amp_veltrack_cc1") == OSC::None);
        REQUIRE( d.read<float>("/region1/amp_veltrack_cc1") == 10.1f);
        REQUIRE( d.read<int32_t>("/region1/amp_veltrack_curvecc1") == 3);
        REQUIRE( d.read<float>("/region2/amp_veltrack_cc2") == -40.0f);
        REQUIRE( d.read<int32_t>("/region2/amp_veltrack_curvecc3") == 4);
    }

    SECTION("Amp Random")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amp_random=10.1
            <region> sample=kick.wav amp_random=-4
        )");
        REQUIRE( d.read<float>("/region0/amp_random") == 0.0f);
        REQUIRE( d.read<float>("/region1/amp_random") == 10.1f);
        REQUIRE( d.read<float>("/region2/amp_random") == -4.0f);
    }

    SECTION("Key Xfin")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_lokey=10 xfin_hikey=40
            <region> sample=kick.wav xfin_lokey=c4 xfin_hikey=b5
            <region> sample=kick.wav xfin_lokey=-10 xfin_hikey=40
            <region> sample=kick.wav xfin_lokey=10 xfin_hikey=140
        )");
        REQUIRE_THAT( d.readAll<int32_t>("/region0/xfin_key_range"), Catch::Approx(std::vector<int32_t>{ 0, 0 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region1/xfin_key_range"), Catch::Approx(std::vector<int32_t>{ 10, 40 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region2/xfin_key_range"), Catch::Approx(std::vector<int32_t>{ 60, 83 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region3/xfin_key_range"), Catch::Approx(std::vector<int32_t>{ 0, 40 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region4/xfin_key_range"), Catch::Approx(std::vector<int32_t>{ 10, 0 }));
    }

    SECTION("Key Xfout")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_lokey=10 xfout_hikey=40
            <region> sample=kick.wav xfout_lokey=c4 xfout_hikey=b5
            <region> sample=kick.wav xfout_lokey=-10 xfout_hikey=40
            <region> sample=kick.wav xfout_lokey=10 xfout_hikey=140
        )");
        REQUIRE_THAT( d.readAll<int32_t>("/region0/xfout_key_range"), Catch::Approx(std::vector<int32_t>{ 127, 127 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region1/xfout_key_range"), Catch::Approx(std::vector<int32_t>{ 10, 40 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region2/xfout_key_range"), Catch::Approx(std::vector<int32_t>{ 60, 83 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region3/xfout_key_range"), Catch::Approx(std::vector<int32_t>{ 127, 40 }));
        REQUIRE_THAT( d.readAll<int32_t>("/region4/xfout_key_range"), Catch::Approx(std::vector<int32_t>{ 10, 127 }));
    }
}
    SECTION("Velocity Xfin")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_lovel=10 xfin_hivel=40
            <region> sample=kick.wav xfin_lovel=-10 xfin_hivel=40
            <region> sample=kick.wav xfin_lovel=10 xfin_hivel=140
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/xfin_vel_range"), Catch::Approx(std::vector<float>{ 0, 0 }));
        REQUIRE_THAT( d.readAll<float>("/region1/xfin_vel_range"), Catch::Approx(std::vector<float>{ 10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/xfin_vel_range"), Catch::Approx(std::vector<float>{ -10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/xfin_vel_range"), Catch::Approx(std::vector<float>{ 10_norm, 140_norm }));
    }

    SECTION("Velocity Xfout")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_lovel=10 xfout_hivel=40
            <region> sample=kick.wav xfout_lovel=-10 xfout_hivel=40
            <region> sample=kick.wav xfout_lovel=10 xfout_hivel=140
        )");
        REQUIRE_THAT( d.readAll<float>("/region0/xfout_vel_range"), Catch::Approx(std::vector<float>{ 1, 1 }));
        REQUIRE_THAT( d.readAll<float>("/region1/xfout_vel_range"), Catch::Approx(std::vector<float>{ 10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/xfout_vel_range"), Catch::Approx(std::vector<float>{ -10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/xfout_vel_range"), Catch::Approx(std::vector<float>{ 10_norm, 140_norm }));
    }

    SECTION("Crossfade key curve")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_keycurve=gain
            <region> sample=kick.wav xf_keycurve=something
            <region> sample=kick.wav xf_keycurve=gain xf_keycurve=power
        )");
        REQUIRE( d.read<std::string>("/region0/xf_keycurve") == "power");
        REQUIRE( d.read<std::string>("/region1/xf_keycurve") == "gain");
        REQUIRE( d.read<std::string>("/region2/xf_keycurve") == "power");
        REQUIRE( d.read<std::string>("/region3/xf_keycurve") == "power");
    }

    SECTION("Velocity")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_velcurve=gain
            <region> sample=kick.wav xf_velcurve=something
            <region> sample=kick.wav xf_velcurve=gain xf_velcurve=power
        )");
        REQUIRE( d.read<std::string>("/region0/xf_velcurve") == "power");
        REQUIRE( d.read<std::string>("/region1/xf_velcurve") == "gain");
        REQUIRE( d.read<std::string>("/region2/xf_velcurve") == "power");
        REQUIRE( d.read<std::string>("/region3/xf_velcurve") == "power");
    }

    SECTION("CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_cccurve=gain
            <region> sample=kick.wav xf_cccurve=something
            <region> sample=kick.wav xf_cccurve=gain xf_cccurve=power
        )");
        REQUIRE( d.read<std::string>("/region0/xf_cccurve") == "power");
        REQUIRE( d.read<std::string>("/region1/xf_cccurve") == "gain");
        REQUIRE( d.read<std::string>("/region2/xf_cccurve") == "power");
        REQUIRE( d.read<std::string>("/region3/xf_cccurve") == "power");
    }

    SECTION("CC Xfin")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_locc4=10 xfin_hicc4=40
            <region> sample=kick.wav xfin_locc4=-10 xfin_hicc4=40
            <region> sample=kick.wav xfin_locc4=10 xfin_hicc4=140
        )");
        REQUIRE( d.read<OSC>("/region0/xfin_cc_range4") == OSC::None );
        REQUIRE_THAT( d.readAll<float>("/region1/xfin_cc_range4"), Catch::Approx(std::vector<float>{ 10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/xfin_cc_range4"), Catch::Approx(std::vector<float>{ -10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/xfin_cc_range4"), Catch::Approx(std::vector<float>{ 10_norm, 140_norm }));
    }

    SECTION("CC Xfout")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_locc4=10 xfout_hicc4=40
            <region> sample=kick.wav xfout_locc4=-10 xfout_hicc4=40
            <region> sample=kick.wav xfout_locc4=10 xfout_hicc4=140
        )");
        REQUIRE( d.read<OSC>("/region0/xfout_cc_range4") == OSC::None );
        REQUIRE_THAT( d.readAll<float>("/region1/xfout_cc_range4"), Catch::Approx(std::vector<float>{ 10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region2/xfout_cc_range4"), Catch::Approx(std::vector<float>{ -10_norm, 41_norm }));
        REQUIRE_THAT( d.readAll<float>("/region3/xfout_cc_range4"), Catch::Approx(std::vector<float>{ 10_norm, 140_norm }));
    }

    SECTION("Global Volume")
    {
        d.load(R"(
            <region> sample=kick.wav
            <global> global_volume=4.4
            <master> master_volume=5.5
            <group> group_volume=6.6
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<float>("/region0/global_volume") == 0.0f);
        REQUIRE( d.read<float>("/region0/master_volume") == 0.0f);
        REQUIRE( d.read<float>("/region0/group_volume") == 0.0f);
        REQUIRE( d.read<float>("/region1/global_volume") == 4.4f);
        REQUIRE( d.read<float>("/region1/master_volume") == 5.5f);
        REQUIRE( d.read<float>("/region1/group_volume") == 6.6f);
    }

    SECTION("Amplitudes")
    {
        d.load(R"(
            <region> sample=kick.wav
            <global> global_amplitude=4.4
            <master> master_amplitude=5.5
            <group> group_amplitude=6.6
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<float>("/region0/global_amplitude") == 100.0f);
        REQUIRE( d.read<float>("/region0/master_amplitude") == 100.0f);
        REQUIRE( d.read<float>("/region0/group_amplitude") == 100.0f);
        REQUIRE( d.read<float>("/region1/global_amplitude") == 4.4f);
        REQUIRE( d.read<float>("/region1/master_amplitude") == 5.5f);
        REQUIRE( d.read<float>("/region1/group_amplitude") == 6.6f);
    }

    SECTION("Pitch Keytrack")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_keytrack=1000
            <region> sample=kick.wav pitch_keytrack=-100
        )");
        REQUIRE( d.read<float>("/region0/pitch_keytrack") == 100.0f);
        REQUIRE( d.read<float>("/region1/pitch_keytrack") == 1000.0f);
        REQUIRE( d.read<float>("/region2/pitch_keytrack") == -100.0f);
    }

    SECTION("Pitch veltrack basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_veltrack=10
            <region> sample=kick.wav pitch_veltrack=-132
        )");
        REQUIRE( d.read<float>("/region0/pitch_veltrack") == 0.0f);
        REQUIRE( d.read<float>("/region1/pitch_veltrack") == 10.0f);
        REQUIRE( d.read<float>("/region2/pitch_veltrack") == -132.0f);
    }

    SECTION("Pitch veltrack CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_veltrack_cc1=10.1 pitch_veltrack_curvecc1=3
            <region> sample=kick.wav pitch_veltrack_oncc2=-40 pitch_veltrack_curvecc3=4
        )");
        REQUIRE( d.read<OSC>("/region0/pitch_veltrack_cc1") == OSC::None);
        REQUIRE( d.read<float>("/region1/pitch_veltrack_cc1") == 10.1f);
        REQUIRE( d.read<int32_t>("/region1/pitch_veltrack_curvecc1") == 3);
        REQUIRE( d.read<float>("/region2/pitch_veltrack_cc2") == -40.0f);
        REQUIRE( d.read<int32_t>("/region2/pitch_veltrack_curvecc3") == 4);
        // TODO: activate for the new region parser ; accept oob
        // REQUIRE( d.read<>("/region2/pitch_veltrack") == );
    }

    SECTION("Pitch Random")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_random=10
            <region> sample=kick.wav pitch_random=-4
        )");
        REQUIRE( d.read<float>("/region0/pitch_random") == 0.0f);
        REQUIRE( d.read<float>("/region1/pitch_random") == 10.0f);
        REQUIRE( d.read<float>("/region2/pitch_random") == -4.0f);
    }

    SECTION("Transpose")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav transpose=10
            <region> sample=kick.wav transpose=-4
            <region> sample=kick.wav transpose=-400
            <region> sample=kick.wav transpose=400
        )");
        REQUIRE( d.read<float>("/region0/transpose") == 0.0f);
        REQUIRE( d.read<float>("/region1/transpose") == 10.0f);
        REQUIRE( d.read<float>("/region2/transpose") == -4.0f);
        REQUIRE( d.read<float>("/region3/transpose") == -400.0f);
        REQUIRE( d.read<float>("/region4/transpose") == 400.0f);
    }

    SECTION("Pitch/tune basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch=4.2
            <region> sample=kick.wav tune=-200
        )");
        REQUIRE( d.read<float>("/region0/pitch") == 0.0f);
        REQUIRE( d.read<float>("/region1/pitch") == 4.2f);
        REQUIRE( d.read<float>("/region2/pitch") == -200.0f);
    }

    SECTION("Pitch/tune CC Depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_oncc42=4.2
            <region> sample=kick.wav pitch_oncc2=-10
        )");
        REQUIRE( d.read<OSC>("/region0/pitch_cc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/pitch_cc42") == 4.2f);
        REQUIRE( d.read<float>("/region2/pitch_cc2") == -10.0f);
    }

    SECTION("Pitch/tune CC Params")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_stepcc42=4.2
            <region> sample=kick.wav pitch_smoothcc42=4
            <region> sample=kick.wav pitch_curvecc42=2
            <region> sample=kick.wav pitch_stepcc42=-1
            <region> sample=kick.wav pitch_smoothcc42=-4
            <region> sample=kick.wav pitch_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/pitch_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitch_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitch_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/pitch_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/pitch_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/pitch_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/pitch_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/pitch_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/pitch_curvecc42") == OSC::None);
    }

    SECTION("Pitch/tune CC Params (with pitch_)")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_stepcc42=4.2
            <region> sample=kick.wav pitch_smoothcc42=4
            <region> sample=kick.wav pitch_curvecc42=2
            <region> sample=kick.wav pitch_stepcc42=-1
            <region> sample=kick.wav pitch_smoothcc42=-4
            <region> sample=kick.wav pitch_curvecc42=300
        )");
        REQUIRE( d.read<OSC>("/region0/pitch_stepcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitch_smoothcc42") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitch_curvecc42") == OSC::None);
        REQUIRE( d.read<float>("/region1/pitch_stepcc42") == 4.2f);
        REQUIRE( d.read<int32_t>("/region2/pitch_smoothcc42") == 4 );
        REQUIRE( d.read<int32_t>("/region3/pitch_curvecc42") == 2);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region4/pitch_stepcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region5/pitch_smoothcc42") == OSC::None);
        // REQUIRE( d.read<OSC>("/region6/pitch_curvecc42") == OSC::None);
    }

    SECTION("Bend behavior")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav bend_up=100 bend_down=-400 bend_step=10 bend_smooth=10
            <region> sample=kick.wav bend_up=-100 bend_down=400 bend_step=-10 bend_smooth=-10
        )");
        REQUIRE( d.read<float>("/region0/bend_up") == 200.0f);
        REQUIRE( d.read<float>("/region0/bend_down") == -200.0f);
        REQUIRE( d.read<float>("/region0/bend_step") == 1.0f);
        REQUIRE( d.read<int32_t>("/region0/bend_smooth") == 0);
        REQUIRE( d.read<float>("/region1/bend_up") == 100.0f);
        REQUIRE( d.read<float>("/region1/bend_down") == -400.0f);
        REQUIRE( d.read<float>("/region1/bend_step") == 10.0f);
        REQUIRE( d.read<int32_t>("/region1/bend_smooth") == 10);
        REQUIRE( d.read<float>("/region2/bend_up") == -100.0f);
        REQUIRE( d.read<float>("/region2/bend_down") == 400.0f);
        REQUIRE( d.read<float>("/region2/bend_step") == 1.0f);
        REQUIRE( d.read<int32_t>("/region2/bend_smooth") == 0);
    }

    SECTION("Ampeg basic")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav
                ampeg_attack=1 ampeg_delay=2 ampeg_decay=3
                ampeg_hold=4 ampeg_release=5 ampeg_start=6
                ampeg_sustain=7 ampeg_depth=8
            <region> sample=kick.wav
                ampeg_attack=-1 ampeg_delay=-2 ampeg_decay=-3
                ampeg_hold=-4 ampeg_release=-5 ampeg_start=-6
                ampeg_sustain=-7 ampeg_depth=-8
        )");
        REQUIRE( d.read<float>("/region0/ampeg_attack") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_delay") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_decay") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_hold") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_release") == 0.001f);
        REQUIRE( d.read<float>("/region0/ampeg_start") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_sustain") == 100.0f);
        REQUIRE( d.read<float>("/region0/ampeg_depth") == 0.0f);
        REQUIRE( d.read<float>("/region1/ampeg_attack") == 1.0f);
        REQUIRE( d.read<float>("/region1/ampeg_delay") == 2.0f);
        REQUIRE( d.read<float>("/region1/ampeg_decay") == 3.0f);
        REQUIRE( d.read<float>("/region1/ampeg_hold") == 4.0f);
        REQUIRE( d.read<float>("/region1/ampeg_release") == 5.0f);
        REQUIRE( d.read<float>("/region1/ampeg_start") == 6.0f);
        REQUIRE( d.read<float>("/region1/ampeg_sustain") == 7.0f);
        REQUIRE( d.read<float>("/region1/ampeg_depth") == 0.0f);
        // TODO after new parser : ignore oob
        // REQUIRE( d.read<float>("/region2/ampeg_attack") == 0.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_delay") == 0.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_decay") == 0.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_hold") == 0.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_release") == 0.001f);
        // REQUIRE( d.read<float>("/region2/ampeg_start") == 0.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_sustain") == 100.0f);
        // REQUIRE( d.read<float>("/region2/ampeg_depth") == 0.0f);
    }

    SECTION("Ampeg velocity")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav
                ampeg_vel2attack=1 ampeg_vel2delay=2 ampeg_vel2decay=3
                ampeg_vel2hold=4 ampeg_vel2release=5
                ampeg_vel2sustain=7 ampeg_vel2depth=8
        )");
        REQUIRE( d.read<float>("/region0/ampeg_vel2attack") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2delay") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2decay") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2hold") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2release") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2sustain") == 0.0f );
        REQUIRE( d.read<float>("/region0/ampeg_vel2depth") == 0.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2attack") == 1.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2delay") == 2.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2decay") == 3.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2hold") == 4.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2release") == 5.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2sustain") == 7.0f );
        REQUIRE( d.read<float>("/region1/ampeg_vel2depth") == 0.0f );
    }

    SECTION("Note polyphony")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav note_polyphony=10
            <region> sample=kick.wav note_polyphony=-4
            <region> sample=kick.wav note_polyphony=10 note_polyphony=-4
        )");
        REQUIRE( d.read<OSC>("/region0/note_polyphony") == OSC::None );
        REQUIRE( d.read<int32_t>("/region1/note_polyphony") == 10);
        // TODO: activate for the new region parser ; ignore oob
        // REQUIRE( d.read<OSC>("/region2/note_polyphony") == OSC::None );
        // REQUIRE( d.read<int32_t>("/region3/note_polyphony") == 10);
    }

    SECTION("Self-mask")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav note_selfmask=off
            <region> sample=kick.wav note_selfmask=off note_selfmask=on
            <region> sample=kick.wav note_selfmask=off note_selfmask=garbage
        )");
        REQUIRE( d.read<OSC>("/region0/note_selfmask") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/note_selfmask") == OSC::False);
        REQUIRE( d.read<OSC>("/region2/note_selfmask") == OSC::True);
        REQUIRE( d.read<OSC>("/region3/note_selfmask") == OSC::True);
    }

    SECTION("RT dead")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav rt_dead=on
            <region> sample=kick.wav rt_dead=on rt_dead=off
            <region> sample=kick.wav rt_dead=on rt_dead=garbage
        )");
        REQUIRE( d.read<OSC>("/region0/rt_dead") == OSC::False);
        REQUIRE( d.read<OSC>("/region1/rt_dead") == OSC::True);
        REQUIRE( d.read<OSC>("/region2/rt_dead") == OSC::False);
        REQUIRE( d.read<OSC>("/region3/rt_dead") == OSC::False);
    }

    SECTION("Sustain switch")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sustain_sw=off
            <region> sample=kick.wav sustain_sw=off sustain_sw=on
            <region> sample=kick.wav sustain_sw=off sustain_sw=garbage
        )");
        REQUIRE( d.read<OSC>("/region0/sustain_sw") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/sustain_sw") == OSC::False);
        REQUIRE( d.read<OSC>("/region2/sustain_sw") == OSC::True);
        REQUIRE( d.read<OSC>("/region3/sustain_sw") == OSC::True);
    }

    SECTION("Sostenuto switch")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sostenuto_sw=off
            <region> sample=kick.wav sostenuto_sw=off sostenuto_sw=on
            <region> sample=kick.wav sostenuto_sw=off sostenuto_sw=garbage
        )");
        REQUIRE( d.read<OSC>("/region0/sostenuto_sw") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/sostenuto_sw") == OSC::False);
        REQUIRE( d.read<OSC>("/region2/sostenuto_sw") == OSC::True);
        REQUIRE( d.read<OSC>("/region3/sostenuto_sw") == OSC::True);
    }

    SECTION("Sustain CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sustain_cc=10
            <region> sample=kick.wav sustain_cc=20 sustain_cc=-1
        )");
        REQUIRE( d.read<int32_t>("/region0/sustain_cc") == 64);
        REQUIRE( d.read<int32_t>("/region1/sustain_cc") == 10);
        REQUIRE( d.read<int32_t>("/region2/sustain_cc") == 64);
    }

    SECTION("Sustain low")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sustain_lo=10
            <region> sample=kick.wav sustain_lo=10 sustain_lo=-1
        )");
        REQUIRE( d.read<float>("/region0/sustain_lo") == 1_norm);
        REQUIRE( d.read<float>("/region1/sustain_lo") == 10_norm);
        REQUIRE( d.read<float>("/region2/sustain_lo") == -1_norm);
    }

    SECTION("Sostenuto CC")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sostenuto_cc=10
            <region> sample=kick.wav sostenuto_cc=20 sostenuto_cc=-1
        )");
        REQUIRE( d.read<int32_t>("/region0/sostenuto_cc") == 66);
        REQUIRE( d.read<int32_t>("/region1/sostenuto_cc") == 10);
        REQUIRE( d.read<int32_t>("/region2/sostenuto_cc") == 66);
    }

    SECTION("Sostenuto low")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sostenuto_lo=10
            <region> sample=kick.wav sostenuto_lo=10 sostenuto_lo=-1
        )");
        REQUIRE( d.read<float>("/region0/sostenuto_lo") == 1_norm);
        REQUIRE( d.read<float>("/region1/sostenuto_lo") == 10_norm);
        REQUIRE( d.read<float>("/region2/sostenuto_lo") == -1_norm);
    }

    SECTION("Oscillator phase")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav oscillator_phase=0.1
            <region> sample=kick.wav oscillator_phase=1.1
            <region> sample=kick.wav oscillator_phase=-1.2
        )");
        REQUIRE( d.read<float>("/region0/oscillator_phase") == 0.0f);
        REQUIRE_THAT( d.read<float>("/region1/oscillator_phase"), Catch::WithinRel(0.1f));
        REQUIRE_THAT( d.read<float>("/region2/oscillator_phase"), Catch::WithinRel(0.1f));
        REQUIRE( d.read<float>("/region3/oscillator_phase") == -1.0f);
    }

    SECTION("Oscillator quality")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav oscillator_quality=2
            <region> sample=kick.wav oscillator_quality=0 oscillator_quality=-2
        )");
        REQUIRE( d.read<OSC>("/region0/oscillator_quality") == OSC::None);
        REQUIRE( d.read<int32_t>("/region1/oscillator_quality") == 2);
        REQUIRE( d.read<OSC>("/region2/oscillator_quality") == OSC::None);
    }

    SECTION("Oscillator mode/multi")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav oscillator_mode=2
            <region> sample=kick.wav oscillator_mode=1 oscillator_mode=-2
            <region> sample=kick.wav oscillator_multi=9
            <region> sample=kick.wav oscillator_multi=-2
        )");
        REQUIRE( d.read<int32_t>("/region0/oscillator_mode") == 0);
        REQUIRE( d.read<int32_t>("/region1/oscillator_mode") == 2);
        REQUIRE( d.read<int32_t>("/region2/oscillator_mode") == 0);
        REQUIRE( d.read<int32_t>("/region0/oscillator_multi") == 1);
        REQUIRE( d.read<int32_t>("/region3/oscillator_multi") == 9);
        REQUIRE( d.read<int32_t>("/region4/oscillator_multi") == 1);
    }

    SECTION("Oscillator detune/mod depth")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav oscillator_detune=9.2
            <region> sample=kick.wav oscillator_detune=-1200.2
            <region> sample=kick.wav oscillator_mod_depth=1564.75
            <region> sample=kick.wav oscillator_mod_depth=-2.2
        )");
        REQUIRE( d.read<float>("/region0/oscillator_detune") == 0.0f);
        REQUIRE( d.read<float>("/region1/oscillator_detune") == 9.2f);
        REQUIRE( d.read<float>("/region2/oscillator_detune") == -1200.2f);
        REQUIRE( d.read<float>("/region0/oscillator_mod_depth") == 0.0f);
        REQUIRE( d.read<float>("/region3/oscillator_mod_depth") == 1564.75f);
        REQUIRE( d.read<float>("/region4/oscillator_mod_depth") == -2.2f);
    }

    SECTION("Effect sends")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav effect1=10
            <region> sample=kick.wav effect2=50.4
            <region> sample=kick.wav effect1=-1
        )");
        REQUIRE( !d.replied("/region0/effect1") );
        REQUIRE( d.read<float>("/region1/effect1") == 10.0f);
        REQUIRE( d.read<float>("/region2/effect1") == 0.0f);
        REQUIRE( d.read<float>("/region2/effect2") == 50.4f);
        REQUIRE( !d.replied("/region4/effect1") );
    }

    SECTION("Support floating point for int values")
    {
        d.load(R"(
            <region> sample=kick.wav offset=1042.5
            <region> sample=kick.wav pitch_keytrack=-2.1
        )");
        REQUIRE( d.read<int64_t>("/region0/offset") == 1042);
        REQUIRE( d.read<float>("/region1/pitch_keytrack") == -2.1f);
    }

    SECTION("ampeg CC Basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<float>("/region0/ampeg_attack_cc1") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_delay_cc2") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_decay_cc3") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_hold_cc4") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_release_cc5") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_start_cc6") == 0.0f);
        REQUIRE( d.read<float>("/region0/ampeg_sustain_cc7") == 0.0f);
    }

    SECTION("ampeg CC Positive values")
    {
        d.load(R"(
            <region> sample=kick.wav
                ampeg_attack_oncc1=1 ampeg_delay_oncc2=2 ampeg_decay_oncc3=3
                ampeg_hold_oncc4=4 ampeg_release_oncc5=5 ampeg_start_oncc6=6
                ampeg_sustain_oncc7=7
        )");
        REQUIRE( d.read<float>("/region0/ampeg_attack_cc1") == 1.0f);
        REQUIRE( d.read<float>("/region0/ampeg_delay_cc2") == 2.0f);
        REQUIRE( d.read<float>("/region0/ampeg_decay_cc3") == 3.0f);
        REQUIRE( d.read<float>("/region0/ampeg_hold_cc4") == 4.0f);
        REQUIRE( d.read<float>("/region0/ampeg_release_cc5") == 5.0f);
        REQUIRE( d.read<float>("/region0/ampeg_start_cc6") == 6.0f);
        REQUIRE( d.read<float>("/region0/ampeg_sustain_cc7") == 7.0f);
    }

    SECTION("ampeg CC Negative values")
    {
        d.load(R"(
            <region> sample=kick.wav
                ampeg_attack_cc1=-1 ampeg_delay_cc2=-2 ampeg_decay_cc3=-3
                ampeg_hold_cc4=-4 ampeg_release_cc5=-5 ampeg_start_cc6=-6
                ampeg_sustain_cc7=-7
        )");
        REQUIRE( d.read<float>("/region0/ampeg_attack_cc1") == -1.0f);
        REQUIRE( d.read<float>("/region0/ampeg_delay_cc2") == -2.0f);
        REQUIRE( d.read<float>("/region0/ampeg_decay_cc3") == -3.0f);
        REQUIRE( d.read<float>("/region0/ampeg_hold_cc4") == -4.0f);
        REQUIRE( d.read<float>("/region0/ampeg_release_cc5") == -5.0f);
        REQUIRE( d.read<float>("/region0/ampeg_start_cc6") == -6.0f);
        REQUIRE( d.read<float>("/region0/ampeg_sustain_cc7") == -7.0f);
    }

    SECTION("fileg basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<OSC>("/region0/fileg_attack_cc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_delay_cc2") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_decay_cc3") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_hold_cc4") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_release_cc5") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_start_cc6") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_sustain_cc7") == OSC::None);
    }

    SECTION("fileg positive values")
    {
        d.load(R"(
            <region> sample=kick.wav
                fileg_attack_oncc1=1 fileg_delay_oncc2=2 fileg_decay_oncc3=3
                fileg_hold_oncc4=4 fileg_release_oncc5=5 fileg_start_oncc6=6
                fileg_sustain_oncc7=7
        )");
        REQUIRE( d.read<float>("/region0/fileg_attack_cc1") == 1.0f);
        REQUIRE( d.read<float>("/region0/fileg_delay_cc2") == 2.0f);
        REQUIRE( d.read<float>("/region0/fileg_decay_cc3") == 3.0f);
        REQUIRE( d.read<float>("/region0/fileg_hold_cc4") == 4.0f);
        REQUIRE( d.read<float>("/region0/fileg_release_cc5") == 5.0f);
        REQUIRE( d.read<float>("/region0/fileg_start_cc6") == 6.0f);
        REQUIRE( d.read<float>("/region0/fileg_sustain_cc7") == 7.0f);
    }

    SECTION("fileg negative values")
    {
        d.load(R"(
            <region> sample=kick.wav
                fileg_attack_cc1=-1 fileg_delay_cc2=-2 fileg_decay_cc3=-3
                fileg_hold_cc4=-4 fileg_release_cc5=-5 fileg_start_cc6=-6
                fileg_sustain_cc7=-7
        )");
        REQUIRE( d.read<float>("/region0/fileg_attack_cc1") == -1.0f);
        REQUIRE( d.read<float>("/region0/fileg_delay_cc2") == -2.0f);
        REQUIRE( d.read<float>("/region0/fileg_decay_cc3") == -3.0f);
        REQUIRE( d.read<float>("/region0/fileg_hold_cc4") == -4.0f);
        REQUIRE( d.read<float>("/region0/fileg_release_cc5") == -5.0f);
        REQUIRE( d.read<float>("/region0/fileg_start_cc6") == -6.0f);
        REQUIRE( d.read<float>("/region0/fileg_sustain_cc7") == -7.0f);
    }

    SECTION("pitcheg basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<OSC>("/region0/pitcheg_attack_cc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_delay_cc2") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_decay_cc3") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_hold_cc4") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_release_cc5") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_start_cc6") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_sustain_cc7") == OSC::None);
    }

    SECTION("pitcheg positive values")
    {
        d.load(R"(
            <region> sample=kick.wav
                pitcheg_attack_oncc1=1 pitcheg_delay_oncc2=2 pitcheg_decay_oncc3=3
                pitcheg_hold_oncc4=4 pitcheg_release_oncc5=5 pitcheg_start_oncc6=6
                pitcheg_sustain_oncc7=7
        )");
        REQUIRE( d.read<float>("/region0/pitcheg_attack_cc1") == 1.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_delay_cc2") == 2.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_decay_cc3") == 3.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_hold_cc4") == 4.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_release_cc5") == 5.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_start_cc6") == 6.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_sustain_cc7") == 7.0f);
    }

    SECTION("pitcheg negative values")
    {
        d.load(R"(
            <region> sample=kick.wav
                pitcheg_attack_cc1=-1 pitcheg_delay_cc2=-2 pitcheg_decay_cc3=-3
                pitcheg_hold_cc4=-4 pitcheg_release_cc5=-5 pitcheg_start_cc6=-6
                pitcheg_sustain_cc7=-7
        )");
        REQUIRE( d.read<float>("/region0/pitcheg_attack_cc1") == -1.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_delay_cc2") == -2.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_decay_cc3") == -3.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_hold_cc4") == -4.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_release_cc5") == -5.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_start_cc6") == -6.0f);
        REQUIRE( d.read<float>("/region0/pitcheg_sustain_cc7") == -7.0f);
    }

    SECTION("ampeg curve CC basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<int32_t>("/region0/ampeg_attack_curvecc1") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_delay_curvecc2") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_decay_curvecc3") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_hold_curvecc4") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_release_curvecc5") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_start_curvecc6") == 0);
        REQUIRE( d.read<int32_t>("/region0/ampeg_sustain_curvecc7") == 0);
    }

    SECTION("ampeg curve CC change curves")
    {
        d.load(R"(
            <region> sample=kick.wav
                ampeg_attack_curvecc1=1 ampeg_delay_curvecc2=2 ampeg_decay_curvecc3=3
                ampeg_hold_curvecc4=4 ampeg_release_curvecc5=5 ampeg_start_curvecc6=6
                ampeg_sustain_curvecc7=7
        )");
        REQUIRE( d.read<int32_t>("/region0/ampeg_attack_curvecc1") == 1);
        REQUIRE( d.read<int32_t>("/region0/ampeg_delay_curvecc2") == 2);
        REQUIRE( d.read<int32_t>("/region0/ampeg_decay_curvecc3") == 3);
        REQUIRE( d.read<int32_t>("/region0/ampeg_hold_curvecc4") == 4);
        REQUIRE( d.read<int32_t>("/region0/ampeg_release_curvecc5") == 5);
        REQUIRE( d.read<int32_t>("/region0/ampeg_start_curvecc6") == 6);
        REQUIRE( d.read<int32_t>("/region0/ampeg_sustain_curvecc7") == 7);
    }

    SECTION("fileg curve CC basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<OSC>("/region0/fileg_attack_curvecc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_delay_curvecc2") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_decay_curvecc3") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_hold_curvecc4") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_release_curvecc5") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_start_curvecc6") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_sustain_curvecc7") == OSC::None);
    }

    SECTION("fileg curve CC change curves")
    {
        d.load(R"(
            <region> sample=kick.wav
                fileg_attack_curvecc1=1 fileg_delay_curvecc2=2 fileg_decay_curvecc3=3
                fileg_hold_curvecc4=4 fileg_release_curvecc5=5 fileg_start_curvecc6=6
                fileg_sustain_curvecc7=7
        )");
        REQUIRE( d.read<int32_t>("/region0/fileg_attack_curvecc1") == 1);
        REQUIRE( d.read<int32_t>("/region0/fileg_delay_curvecc2") == 2);
        REQUIRE( d.read<int32_t>("/region0/fileg_decay_curvecc3") == 3);
        REQUIRE( d.read<int32_t>("/region0/fileg_hold_curvecc4") == 4);
        REQUIRE( d.read<int32_t>("/region0/fileg_release_curvecc5") == 5);
        REQUIRE( d.read<int32_t>("/region0/fileg_start_curvecc6") == 6);
        REQUIRE( d.read<int32_t>("/region0/fileg_sustain_curvecc7") == 7);
    }

    SECTION("pitcheg curve CC basic")
    {
        d.load(R"(
            <region> sample=kick.wav
        )");
        REQUIRE( d.read<OSC>("/region0/pitcheg_attack_curvecc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_delay_curvecc2") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_decay_curvecc3") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_hold_curvecc4") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_release_curvecc5") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_start_curvecc6") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/pitcheg_sustain_curvecc7") == OSC::None);
    }

    SECTION("pitcheg curve CC change curves")
    {
        d.load(R"(
            <region> sample=kick.wav
                pitcheg_attack_curvecc1=1 pitcheg_delay_curvecc2=2 pitcheg_decay_curvecc3=3
                pitcheg_hold_curvecc4=4 pitcheg_release_curvecc5=5 pitcheg_start_curvecc6=6
                pitcheg_sustain_curvecc7=7
        )");
        REQUIRE( d.read<int32_t>("/region0/pitcheg_attack_curvecc1") == 1);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_delay_curvecc2") == 2);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_decay_curvecc3") == 3);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_hold_curvecc4") == 4);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_release_curvecc5") == 5);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_start_curvecc6") == 6);
        REQUIRE( d.read<int32_t>("/region0/pitcheg_sustain_curvecc7") == 7);
    }

    SECTION("Filter stacking and cutoffs")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav cutoff=50
            <region> sample=kick.wav cutoff2=500
        )");
        REQUIRE( !d.replied("/region0/filter0/cutoff") );
        REQUIRE( !d.replied("/region0/filter0/gain") );
        REQUIRE( !d.replied("/region0/filter0/resonance") );
        REQUIRE( !d.replied("/region0/filter0/keycenter") );
        REQUIRE( !d.replied("/region0/filter0/keytrack") );
        REQUIRE( !d.replied("/region0/filter0/veltrack") );
        REQUIRE( !d.replied("/region0/filter0/type") );
        REQUIRE( !d.replied("/region0/filter1/cutoff") );
        REQUIRE( !d.replied("/region0/filter1/gain") );
        REQUIRE( !d.replied("/region0/filter1/resonance") );
        REQUIRE( !d.replied("/region0/filter1/keycenter") );
        REQUIRE( !d.replied("/region0/filter1/keytrack") );
        REQUIRE( !d.replied("/region0/filter1/veltrack") );
        REQUIRE( !d.replied("/region0/filter1/type") );\

        // Second region
        REQUIRE( d.read<float>("/region1/filter0/cutoff") == 50.0f);
        REQUIRE( d.read<float>("/region1/filter0/gain") == 0.0f);
        REQUIRE( d.read<float>("/region1/filter0/resonance") == 0.0f);
        REQUIRE( d.read<int32_t>("/region1/filter0/keycenter") == 60);
        REQUIRE( d.read<float>("/region1/filter0/keytrack") == 0.0f);
        REQUIRE( d.read<float>("/region1/filter0/veltrack") == 0.0f);
        REQUIRE( d.read<std::string>("/region1/filter0/type") == "lpf_2p");
        // No second filter on the second region
        REQUIRE( !d.replied("/region1/filter1/cutoff"));
        REQUIRE( !d.replied("/region1/filter1/gain"));
        REQUIRE( !d.replied("/region1/filter1/resonance"));
        REQUIRE( !d.replied("/region1/filter1/keycenter"));
        REQUIRE( !d.replied("/region1/filter1/keytrack"));
        REQUIRE( !d.replied("/region1/filter1/veltrack"));
        REQUIRE( !d.replied("/region1/filter1/type"));

        // Third region
        REQUIRE( d.read<float>("/region2/filter0/cutoff") == 0.0f);
        REQUIRE( d.read<float>("/region2/filter0/gain") == 0.0f);
        REQUIRE( d.read<float>("/region2/filter0/resonance") == 0.0f);
        REQUIRE( d.read<int32_t>("/region2/filter0/keycenter") == 60);
        REQUIRE( d.read<float>("/region2/filter0/keytrack") == 0.0f);
        REQUIRE( d.read<float>("/region2/filter0/veltrack") == 0.0f);
        REQUIRE( d.read<std::string>("/region2/filter0/type") == "lpf_2p");
        REQUIRE( d.read<float>("/region2/filter1/cutoff") == 500.0f);
        REQUIRE( d.read<float>("/region2/filter1/gain") == 0.0f);
        REQUIRE( d.read<float>("/region2/filter1/resonance") == 0.0f);
        REQUIRE( d.read<int32_t>("/region2/filter1/keycenter") == 60);
        REQUIRE( d.read<float>("/region2/filter1/keytrack") == 0.0f);
        REQUIRE( d.read<float>("/region2/filter1/veltrack") == 0.0f);
        REQUIRE( d.read<std::string>("/region2/filter1/type") == "lpf_2p");
    }

    SECTION("Cutoff modifiers")
    {
        d.load(R"(
            <region> sample=kick.wav cutoff_cc2=1000 cutoff_stepcc2=10 cutoff_smoothcc2=2 cutoff_curvecc2=4
            <region> sample=kick.wav cutoff2_cc3=100 cutoff2_stepcc3=1 cutoff2_smoothcc3=20 cutoff2_curvecc3=3
        )");

        REQUIRE( d.read<OSC>("/region0/filter0/cutoff_cc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/filter0/cutoff_stepcc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/filter0/cutoff_smoothcc1") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/filter0/cutoff_curvecc1") == OSC::None);
        REQUIRE( d.read<float>("/region0/filter0/cutoff_cc2") == 1000.0f);
        REQUIRE( d.read<float>("/region0/filter0/cutoff_stepcc2") == 10.0f);
        REQUIRE( d.read<int32_t>("/region0/filter0/cutoff_smoothcc2") == 2);
        REQUIRE( d.read<int32_t>("/region0/filter0/cutoff_curvecc2") == 4);
        REQUIRE( d.read<float>("/region1/filter1/cutoff_cc3") == 100.0f);
        REQUIRE( d.read<float>("/region1/filter1/cutoff_stepcc3") == 1.0f);
        REQUIRE( d.read<int32_t>("/region1/filter1/cutoff_smoothcc3") == 20);
        REQUIRE( d.read<int32_t>("/region1/filter1/cutoff_curvecc3") == 3);
    }

    SECTION("Filter types")
    {
        d.load(R"(
            <region> sample=kick.wav fil_type=lpf_1p
            <region> sample=kick.wav fil_type=hpf_1p
            <region> sample=kick.wav fil_type=lpf_2p
            <region> sample=kick.wav fil_type=hpf_2p
            <region> sample=kick.wav fil_type=bpf_2p
            <region> sample=kick.wav fil_type=brf_2p
            <region> sample=kick.wav fil_type=bpf_1p
            <region> sample=kick.wav fil_type=brf_1p
            <region> sample=kick.wav fil_type=apf_1p
            <region> sample=kick.wav fil_type=lpf_2p_sv
            <region> sample=kick.wav fil_type=hpf_2p_sv
            <region> sample=kick.wav fil_type=bpf_2p_sv
            <region> sample=kick.wav fil_type=brf_2p_sv
            <region> sample=kick.wav fil_type=lpf_4p
            <region> sample=kick.wav fil_type=hpf_4p
            <region> sample=kick.wav fil_type=lpf_6p
            <region> sample=kick.wav fil_type=hpf_6p
            <region> sample=kick.wav fil_type=pink
            <region> sample=kick.wav fil_type=lsh
            <region> sample=kick.wav fil_type=hsh
            <region> sample=kick.wav fil_type=peq
            <region> sample=kick.wav fil2_type=peq
            <region> sample=kick.wav fil2_type=something
        )");

        REQUIRE( d.read<std::string>("/region0/filter0/type") == "lpf_1p" );
        REQUIRE( d.read<std::string>("/region1/filter0/type") == "hpf_1p" );
        REQUIRE( d.read<std::string>("/region2/filter0/type") == "lpf_2p" );
        REQUIRE( d.read<std::string>("/region3/filter0/type") == "hpf_2p" );
        REQUIRE( d.read<std::string>("/region4/filter0/type") == "bpf_2p" );
        REQUIRE( d.read<std::string>("/region5/filter0/type") == "brf_2p" );
        REQUIRE( d.read<std::string>("/region6/filter0/type") == "bpf_1p" );
        REQUIRE( d.read<std::string>("/region7/filter0/type") == "brf_2p" ); // If we have a 1-pole brf at one point, change it back
        REQUIRE( d.read<std::string>("/region8/filter0/type") == "none" ); // If the apf 1-pole works, change it back
        REQUIRE( d.read<std::string>("/region9/filter0/type") == "lpf_2p_sv" );
        REQUIRE( d.read<std::string>("/region10/filter0/type") == "hpf_2p_sv" );
        REQUIRE( d.read<std::string>("/region11/filter0/type") == "bpf_2p_sv" );
        REQUIRE( d.read<std::string>("/region12/filter0/type") == "brf_2p_sv" );
        REQUIRE( d.read<std::string>("/region13/filter0/type") == "lpf_4p" );
        REQUIRE( d.read<std::string>("/region14/filter0/type") == "hpf_4p" );
        REQUIRE( d.read<std::string>("/region15/filter0/type") == "lpf_6p" );
        REQUIRE( d.read<std::string>("/region16/filter0/type") == "hpf_6p" );
        REQUIRE( d.read<std::string>("/region17/filter0/type") == "pink" );
        REQUIRE( d.read<std::string>("/region18/filter0/type") == "lsh" );
        REQUIRE( d.read<std::string>("/region19/filter0/type") == "hsh" );
        REQUIRE( d.read<std::string>("/region20/filter0/type") == "peq" );
        REQUIRE( d.read<std::string>("/region21/filter1/type") == "peq" );
        REQUIRE( d.read<std::string>("/region22/filter1/type") == "none" );
    }

    SECTION("Filter dispatching")
    {
        d.load(R"(
            <region> sample=kick.wav
                cutoff3=50 resonance2=3 fil2_gain=-5 fil3_keytrack=100
                fil_gain=5 fil1_gain=-5 fil2_veltrack=-100
                fil4_veltrack_cc7=-100 fil5_veltrack_curvecc2=2
        )");

        REQUIRE( d.read<float>("/region0/filter2/cutoff") == 50.0f);
        REQUIRE( d.read<float>("/region0/filter1/resonance") == 3.0f);
        REQUIRE( d.read<float>("/region0/filter1/gain") == -5.0f);
        REQUIRE( d.read<float>("/region0/filter2/keytrack") == 100.0f);
        REQUIRE( d.read<float>("/region0/filter0/gain") == -5.0f);
        REQUIRE( d.read<float>("/region0/filter1/veltrack") == -100.0f);
        REQUIRE( d.read<float>("/region0/filter3/veltrack_cc7") == -100.0f);
        REQUIRE( d.read<int32_t>("/region0/filter4/veltrack_curvecc2") == 2);
    }

    SECTION("Filter value bounds")
    {
        d.load(R"(
            <region> sample=kick.wav cutoff=100000
            <region> sample=kick.wav cutoff=50 cutoff=-100
        )");
        REQUIRE( d.read<float>("/region0/filter0/cutoff") == 100000.0f);
        REQUIRE( d.read<float>("/region1/filter0/cutoff") == -100.0f);
     
        d.load(R"(
            <region> sample=kick.wav resonance=5 resonance=-5
        )");
        REQUIRE( d.read<float>("/region0/filter0/resonance") == -5.0f);
     
        d.load(R"(
            <region> sample=kick.wav fil_keycenter=40
            <region> sample=kick.wav fil_keycenter=40 fil_keycenter=1000
            <region> sample=kick.wav fil_keycenter=c3
        )");
        REQUIRE( d.read<int32_t>("/region0/filter0/keycenter") == 40);
        REQUIRE( d.read<int32_t>("/region1/filter0/keycenter") == 60);
        REQUIRE( d.read<int32_t>("/region2/filter0/keycenter") == 48);
    }

    SECTION("EQ stacking and gains")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav eq1_gain=3
            <region> sample=kick.wav eq4_gain=6
        )");

        REQUIRE( !d.replied("/region0/eq0/gain") );
        REQUIRE( !d.replied("/region0/eq0/type") );
        REQUIRE( !d.replied("/region0/eq0/bandwidth") );
        REQUIRE( !d.replied("/region0/eq0/frequency") );
        REQUIRE( !d.replied("/region0/eq0/vel2gain") );
        REQUIRE( !d.replied("/region0/eq0/vel2freq") );
        REQUIRE( !d.replied("/region0/eq1/gain") );
        REQUIRE( !d.replied("/region0/eq1/type") );
        REQUIRE( !d.replied("/region0/eq1/bandwidth") );
        REQUIRE( !d.replied("/region0/eq1/frequency") );
        REQUIRE( !d.replied("/region0/eq1/vel2gain") );
        REQUIRE( !d.replied("/region0/eq1/vel2freq") );

        REQUIRE( d.read<float>("/region1/eq0/gain") == 3.0f);
        REQUIRE( d.read<std::string>("/region1/eq0/type") == "peak");
        REQUIRE( d.read<float>("/region1/eq0/bandwidth") == 1.0f);
        REQUIRE( d.read<float>("/region1/eq0/frequency") == 50.0f);
        REQUIRE( d.read<float>("/region1/eq0/vel2gain") == 0.0f);
        REQUIRE( d.read<float>("/region1/eq0/vel2freq") == 0.0f);
        REQUIRE( !d.replied("/region1/eq1/gain") );
        REQUIRE( !d.replied("/region1/eq1/type") );
        REQUIRE( !d.replied("/region1/eq1/bandwidth") );
        REQUIRE( !d.replied("/region1/eq1/frequency") );
        REQUIRE( !d.replied("/region1/eq1/vel2gain") );
        REQUIRE( !d.replied("/region1/eq1/vel2freq") );

        // The first eq is default-filled
        REQUIRE( d.read<float>("/region2/eq0/gain") == 0.0f);
        REQUIRE( d.read<std::string>("/region2/eq0/type") == "peak");
        REQUIRE( d.read<float>("/region2/eq0/bandwidth") == 1.0f);
        REQUIRE( d.read<float>("/region2/eq0/frequency") == 50.0f);
        REQUIRE( d.read<float>("/region2/eq0/vel2gain") == 0.0f);
        REQUIRE( d.read<float>("/region2/eq0/vel2freq") == 0.0f);
        REQUIRE( d.read<float>("/region2/eq3/gain") == 6.0f);
        REQUIRE( d.read<std::string>("/region2/eq3/type") == "peak");
        REQUIRE( d.read<float>("/region2/eq3/bandwidth") == 1.0f);
        REQUIRE( d.read<float>("/region2/eq3/frequency") == 0.0f);
        REQUIRE( d.read<float>("/region2/eq3/vel2gain") == 0.0f);
        REQUIRE( d.read<float>("/region2/eq3/vel2freq") == 0.0f);
        REQUIRE( d.read<float>("/region2/eq1/frequency") == 500.0f);
        REQUIRE( d.read<float>("/region2/eq2/frequency") == 5000.0f);
    }

    SECTION("EQ types")
    {
        d.load(R"(
            <region> sample=kick.wav eq1_type=hshelf
            <region> sample=kick.wav eq1_type=lshelf
            <region> sample=kick.wav eq1_type=hshelf eq1_type=peak
            <region> sample=kick.wav eq1_type=something
        )");

        REQUIRE( d.read<std::string>("/region0/eq0/type") == "hshelf");
        REQUIRE( d.read<std::string>("/region1/eq0/type") == "lshelf");
        REQUIRE( d.read<std::string>("/region2/eq0/type") == "peak");
        REQUIRE( d.read<std::string>("/region3/eq0/type") == "none");
    }

    SECTION("EQ dispatching")
    {
        d.load(R"(
            <region> sample=kick.wav
                eq3_bw=2 eq1_gain=-25 eq2_freq=300 eq3_type=lshelf
                eq3_vel2gain=10 eq1_vel2freq=100
        )");

        REQUIRE( d.read<float>("/region0/eq2/bandwidth") == 2.0f);
        REQUIRE( d.read<float>("/region0/eq0/gain") == -25.0f);
        REQUIRE( d.read<float>("/region0/eq1/frequency") == 300.0f);
        REQUIRE( d.read<std::string>("/region0/eq2/type") == "lshelf");
        REQUIRE( d.read<float>("/region0/eq2/vel2gain") == 10.0f);
        REQUIRE( d.read<float>("/region0/eq0/vel2freq") == 100.0f);
    }

    SECTION("EQ value bounds")
    {
        d.load(R"(
            <region> sample=kick.wav eq1_freq=100000
            <region> sample=kick.wav eq1_freq=50 eq1_freq=-100
        )");
        REQUIRE( d.read<float>("/region0/eq0/frequency") == 100000.0f);
        REQUIRE( d.read<float>("/region1/eq0/frequency") == -100.0f);
     
        d.load(R"(
            <region> sample=kick.wav eq1_bw=5 eq1_bw=-5
        )");
        REQUIRE( d.read<float>("/region0/eq0/bandwidth") == -5.0f);
    }

    SECTION("Flex EGs")
    {
        d.load(R"(
            <region> sample=kick.wav eg1_time1=0.1 eg1_level1=0.5 eg1_time2=0.4 eg1_level2=2 eg2_time1=4 eg2_level1=0.1
        )");
        REQUIRE( d.read<float>("/region0/eg0/point0/time") == 0.1f);
        REQUIRE( d.read<float>("/region0/eg0/point0/level") == 0.5f);
        REQUIRE( d.read<float>("/region0/eg0/point1/time") == 0.4f);
        REQUIRE( d.read<float>("/region0/eg0/point1/level") == 1.0f); // Level values in EGs are clamped in Sforzando
        REQUIRE( d.read<float>("/region0/eg1/point0/time") == 4.0f);
        REQUIRE( d.read<float>("/region0/eg1/point0/level") == 0.1f);
    }

    SECTION("Flex EGs CC")
    {
        d.load(R"(
            <region> sample=kick.wav eg1_time1_cc2=0.1 eg1_level1_oncc3=0.5
        )");
        REQUIRE( d.read<float>("/region0/eg0/point0/time_cc2") == 0.1f);
        REQUIRE( d.read<float>("/region0/eg0/point0/time_cc4") == 0.0f);
        REQUIRE( d.read<float>("/region0/eg0/point0/level_cc3") == 0.5f);
        REQUIRE( d.read<float>("/region0/eg0/point0/level_cc12") == 0.0f);
    }

    SECTION("Dynamic EGs")
    {
        d.load(R"(
            <region> sample=kick.wav
            <region> sample=kick.wav ampeg_dynamic=1 pitcheg_dynamic=1 fileg_dynamic=1
        )");
        REQUIRE( d.read<OSC>("/region0/ampeg_dynamic") == OSC::False);
        REQUIRE( d.read<OSC>("/region0/pitcheg_dynamic") == OSC::None);
        REQUIRE( d.read<OSC>("/region0/fileg_dynamic") == OSC::None);
        REQUIRE( d.read<OSC>("/region1/ampeg_dynamic") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/pitcheg_dynamic") == OSC::True);
        REQUIRE( d.read<OSC>("/region1/fileg_dynamic") == OSC::True);
    }
}
