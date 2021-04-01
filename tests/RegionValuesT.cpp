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
using namespace Catch::literals;
using namespace sfz;

TEST_CASE("[Values] Delay")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=*sine
            <region> sample=*sine delay=1
            <region> sample=*sine delay=-1
            <region> sample=*sine delay=1 delay=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/delay", "", nullptr);
        std::vector<std::string> expected {
            "/region0/delay,f : { 0 }",
            "/region1/delay,f : { 1 }",
            "/region2/delay,f : { -1 }",
            "/region3/delay,f : { -1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Random")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=*sine
            <region> sample=*sine delay_random=1
            <region> sample=*sine delay_random=-1
            <region> sample=*sine delay_random=1 delay_random=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/delay_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/delay_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/delay_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/delay_random", "", nullptr);
        std::vector<std::string> expected {
            "/region0/delay_random,f : { 0 }",
            "/region1/delay_random,f : { 1 }",
            "/region2/delay_random,f : { -1 }",
            "/region3/delay_random,f : { -1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav delay_cc12=1.5
            <region> sample=kick.wav delay_cc12=-1.5
            <region> sample=kick.wav delay_cc14=3 delay_cc12=2 delay_cc12=-12
        )");
        synth.dispatchMessage(client, 0, "/region0/delay_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/delay_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/delay_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/delay_cc14", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/delay_cc12", "", nullptr);
        std::vector<std::string> expected {
            "/region0/delay_cc12,f : { 0 }",
            "/region1/delay_cc12,f : { 1.5 }",
            "/region2/delay_cc12,f : { -1.5 }",
            "/region3/delay_cc14,f : { 3 }",
            "/region3/delay_cc12,f : { -12 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Sample and direction")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=*sine
        <region> sample=kick.wav
        <region> sample=kick.wav direction=reverse
    )");
    synth.dispatchMessage(client, 0, "/region0/sample", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sample", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/direction", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/direction", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sample,s : { *sine }",
        "/region1/sample,s : { kick.wav }",
        "/region1/direction,s : { forward }",
        "/region2/direction,s : { reverse }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Offset")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset=12
            <region> sample=kick.wav offset=-1
            <region> sample=kick.wav offset=12 offset=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/offset", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/offset", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/offset", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/offset", "", nullptr);
        std::vector<std::string> expected {
            "/region0/offset,h : { 0 }",
            "/region1/offset,h : { 12 }",
            "/region2/offset,h : { -1 }",
            "/region3/offset,h : { -1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Random")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset_random=1
            <region> sample=kick.wav offset_random=-1
            <region> sample=kick.wav offset_random=1 offset_random=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/offset_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/offset_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/offset_random", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/offset_random", "", nullptr);
        std::vector<std::string> expected {
            "/region0/offset_random,h : { 0 }",
            "/region1/offset_random,h : { 1 }",
            "/region2/offset_random,h : { -1 }",
            "/region3/offset_random,h : { -1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav offset_cc12=12
            <region> sample=kick.wav offset_cc12=-12
            <region> sample=kick.wav offset_cc14=14 offset_cc12=12 offset_cc12=-12
        )");
        synth.dispatchMessage(client, 0, "/region0/offset_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/offset_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/offset_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/offset_cc14", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/offset_cc12", "", nullptr);
        std::vector<std::string> expected {
            "/region0/offset_cc12,h : { 0 }",
            "/region1/offset_cc12,h : { 12 }",
            "/region2/offset_cc12,h : { -12 }",
            "/region3/offset_cc14,h : { 14 }",
            "/region3/offset_cc12,h : { -12 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] End")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav end=194
            <region> sample=kick.wav end=-1
            <region> sample=kick.wav end=0
            <region> sample=kick.wav end=194 end=-1
            <region> sample=kick.wav end=0 end=194
        )");
        synth.dispatchMessage(client, 0, "/region0/end", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/enabled", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/enabled", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/enabled", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/enabled", "", nullptr);
        synth.dispatchMessage(client, 0, "/region4/enabled", "", nullptr);
        synth.dispatchMessage(client, 0, "/region4/end", "", nullptr);
        std::vector<std::string> expected {
            "/region0/end,h : { 194 }",
            "/region0/enabled,T : {  }",
            "/region1/enabled,F : {  }",
            "/region2/enabled,F : {  }",
            "/region3/enabled,F : {  }",
            "/region4/enabled,T : {  }",
            "/region4/end,h : { 194 }",
        };
        REQUIRE(messageList == expected);
    }
    SECTION("CC")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav end_cc12=12
            <region> sample=kick.wav end_oncc12=-12
            <region> sample=kick.wav end_cc14=14 end_cc12=12 end_oncc12=-12
        )");
        synth.dispatchMessage(client, 0, "/region0/end_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/end_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/end_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/end_cc14", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/end_cc12", "", nullptr);
        std::vector<std::string> expected {
            "/region0/end_cc12,h : { 0 }",
            "/region1/end_cc12,h : { 12 }",
            "/region2/end_cc12,h : { -12 }",
            "/region3/end_cc14,h : { 14 }",
            "/region3/end_cc12,h : { -12 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Count")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav count=2
        <region> sample=kick.wav count=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/count", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/count", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/count", "", nullptr);
    std::vector<std::string> expected {
        "/region0/count,N : {  }",
        "/region1/count,h : { 2 }",
        "/region2/count,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Loop mode")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav loop_mode=one_shot
        <region> sample=kick.wav loopmode=one_shot
        <region> sample=kick.wav loop_mode=loop_sustain
        <region> sample=kick.wav loop_mode=loop_continuous
        <region> sample=kick.wav loop_mode=loop_continuous loop_mode=no_loop
    )");
    synth.dispatchMessage(client, 0, "/region0/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/loop_mode", "", nullptr);
    std::vector<std::string> expected {
        "/region0/loop_mode,s : { no_loop }",
        "/region1/loop_mode,s : { one_shot }",
        "/region2/loop_mode,s : { one_shot }",
        "/region3/loop_mode,s : { loop_sustain }",
        "/region4/loop_mode,s : { loop_continuous }",
        "/region5/loop_mode,s : { no_loop }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Loops")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav loop_mode=one_shot
        <region> sample=kick.wav loopmode=one_shot
        <region> sample=kick.wav loop_mode=loop_sustain
        <region> sample=kick.wav loop_mode=loop_continuous
        <region> sample=kick.wav loop_mode=loop_continuous loop_mode=no_loop
    )");
    synth.dispatchMessage(client, 0, "/region0/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/loop_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/loop_mode", "", nullptr);
    std::vector<std::string> expected {
        "/region0/loop_mode,s : { no_loop }",
        "/region1/loop_mode,s : { one_shot }",
        "/region2/loop_mode,s : { one_shot }",
        "/region3/loop_mode,s : { loop_sustain }",
        "/region4/loop_mode,s : { loop_continuous }",
        "/region5/loop_mode,s : { no_loop }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Loop range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_start=10 loop_end=100
            <region> sample=kick.wav loopstart=10 loopend=100
            <region> sample=kick.wav loop_start=-1 loopend=-100
        )");
        synth.dispatchMessage(client, 0, "/region0/loop_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/loop_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/loop_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/loop_range", "", nullptr);
        std::vector<std::string> expected {
            "/region0/loop_range,hh : { 0, 44011 }", // Default loop points in the file
            "/region1/loop_range,hh : { 10, 100 }",
            "/region2/loop_range,hh : { 10, 100 }",
            "/region3/loop_range,hh : { 0, 44011 }",
        };
        REQUIRE(messageList == expected);
    }
    SECTION("CC")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav loop_start_cc12=10 loop_end_cc14=-100
            <region> sample=kick.wav loop_start_oncc12=-10 loop_end_oncc14=100
        )");
        synth.dispatchMessage(client, 0, "/region0/loop_start_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/loop_end_cc14", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/loop_start_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/loop_end_cc14", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/loop_start_cc12", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/loop_end_cc14", "", nullptr);
        std::vector<std::string> expected {
            "/region0/loop_start_cc12,h : { 0 }",
            "/region0/loop_end_cc14,h : { 0 }",
            "/region1/loop_start_cc12,h : { 10 }",
            "/region1/loop_end_cc14,h : { -100 }",
            "/region2/loop_start_cc12,h : { -10 }",
            "/region2/loop_end_cc14,h : { 100 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Loop crossfade")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav loop_crossfade=0.5
        <region> sample=kick.wav loop_crossfade=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/loop_crossfade", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/loop_crossfade", "", nullptr);
    std::vector<std::string> expected {
        "/region0/loop_crossfade,f : { 0.5 }",
        "/region1/loop_crossfade,f : { 0.001 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Loop count")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav loop_count=2
        <region> sample=kick.wav loop_count=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/loop_count", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/loop_count", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/loop_count", "", nullptr);
    std::vector<std::string> expected {
        "/region0/loop_count,N : {  }",
        "/region1/loop_count,h : { 2 }",
        "/region2/loop_count,N : {  }",
    };
    REQUIRE(messageList == expected);
}


TEST_CASE("[Values] Group")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav group=5
        <region> sample=kick.wav group=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/group", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/group", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/group", "", nullptr);
    std::vector<std::string> expected {
        "/region0/group,h : { 0 }",
        "/region1/group,h : { 5 }",
        "/region2/group,h : { 0 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Off by")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav off_by=5
        <region> sample=kick.wav off_by=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/off_by", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/off_by", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/off_by", "", nullptr);
    std::vector<std::string> expected {
        "/region0/off_by,N : {  }",
        "/region1/off_by,h : { 5 }",
        "/region2/off_by,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Off mode")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav off_mode=fast
        <region> sample=kick.wav off_mode=normal
        <region> sample=kick.wav off_mode=time
        <region> sample=kick.wav off_mode=time off_mode=normal
        <region> sample=kick.wav off_mode=nothing
    )");
    synth.dispatchMessage(client, 0, "/region0/off_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/off_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/off_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/off_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/off_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/off_mode", "", nullptr);
    std::vector<std::string> expected {
        "/region0/off_mode,s : { fast }",
        "/region1/off_mode,s : { fast }",
        "/region2/off_mode,s : { normal }",
        "/region3/off_mode,s : { time }",
        "/region4/off_mode,s : { normal }",
        "/region5/off_mode,s : { fast }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Off time")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav off_time=0.1
        <region> sample=kick.wav off_time=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/off_time", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/off_time", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/off_time", "", nullptr);
    std::vector<std::string> expected {
        "/region0/off_time,f : { 0.006 }",
        "/region1/off_time,f : { 0.1 }",
        "/region2/off_time,f : { -1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Key range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lokey=34 hikey=60
        <region> sample=kick.wav lokey=c4 hikey=b5
        <region> sample=kick.wav lokey=-3 hikey=60
        <region> sample=kick.wav hikey=-1
        <region> sample=kick.wav pitch_keycenter=32
        <region> sample=kick.wav pitch_keycenter=-1
        <region> sample=kick.wav key=26
    )");
    synth.dispatchMessage(client, 0, "/region0/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/pitch_keycenter", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/pitch_keycenter", "", nullptr);
    synth.dispatchMessage(client, 0, "/region6/pitch_keycenter", "", nullptr);
    synth.dispatchMessage(client, 0, "/region7/key_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region7/pitch_keycenter", "", nullptr);
    std::vector<std::string> expected {
        "/region0/key_range,ii : { 0, 127 }",
        "/region1/key_range,ii : { 34, 60 }",
        "/region2/key_range,ii : { 60, 83 }",
        "/region3/key_range,ii : { 0, 60 }",
        "/region4/key_range,ii : { 0, 127 }",
        "/region0/pitch_keycenter,i : { 60 }",
        "/region5/pitch_keycenter,i : { 32 }",
        "/region6/pitch_keycenter,i : { 60 }",
        "/region7/key_range,ii : { 26, 26 }",
        "/region7/pitch_keycenter,i : { 26 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Triggers on note")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav hikey=-1
        <region> sample=kick.wav key=-1
        <region> sample=kick.wav hikey=-1 lokey=12
    )");
    synth.dispatchMessage(client, 0, "/region0/trigger_on_note", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/trigger_on_note", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/trigger_on_note", "", nullptr);
    // TODO: Double check with Sforzando/rgc
    synth.dispatchMessage(client, 0, "/region3/trigger_on_note", "", nullptr);
    std::vector<std::string> expected {
        "/region0/trigger_on_note,T : {  }",
        "/region1/trigger_on_note,F : {  }",
        "/region2/trigger_on_note,F : {  }",
        "/region3/trigger_on_note,T : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Velocity range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lovel=34 hivel=60
        <region> sample=kick.wav lovel=-3 hivel=60
        <region> sample=kick.wav hivel=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/vel_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/vel_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/vel_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/vel_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/vel_range,ff : { 0, 1 }",
        "/region1/vel_range,ff : { 0.267717, 0.472441 }",
        "/region2/vel_range,ff : { -0.023622, 0.472441 }",
        "/region3/vel_range,ff : { 0, -0.00787402 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Bend range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lobend=891 hibend=2000
        <region> sample=kick.wav lobend=-891 hibend=891
        <region> sample=kick.wav hibend=-10000
    )");
    synth.dispatchMessage(client, 0, "/region0/bend_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bend_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bend_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/bend_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/bend_range,ff : { -1, 1 }",
        "/region1/bend_range,ff : { 0.108778, 0.24417 }",
        "/region2/bend_range,ff : { -0.108778, 0.108778 }",
        "/region3/bend_range,ff : { -1, -1.22085 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] CC condition range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav locc1=0 hicc1=54
            <region> sample=kick.wav locc1=0 hicc1=54 locc2=2 hicc2=10
            <region> sample=kick.wav locc1=10 hicc1=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/cc_range1", "", nullptr);
        std::vector<std::string> expected {
            "/region0/cc_range1,ff : { 0, 1 }",
            "/region1/cc_range1,ff : { 0, 0.425197 }",
            "/region2/cc_range1,ff : { 0, 0.425197 }",
            "/region2/cc_range2,ff : { 0.015748, 0.0787402 }",
            "/region3/cc_range1,ff : { 0.0787402, -0.00787402 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("hdcc")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lohdcc1=0 hihdcc1=0.1
            <region> sample=kick.wav lohdcc1=0 hihdcc1=0.1 lohdcc2=0.1 hihdcc2=0.2
            <region> sample=kick.wav lohdcc1=0.1 hihdcc1=-0.1
        )");
        synth.dispatchMessage(client, 0, "/region0/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/cc_range1", "", nullptr);
        std::vector<std::string> expected {
            "/region0/cc_range1,ff : { 0, 1 }",
            "/region1/cc_range1,ff : { 0, 0.1 }",
            "/region2/cc_range1,ff : { 0, 0.1 }",
            "/region2/cc_range2,ff : { 0.1, 0.2 }",
            "/region3/cc_range1,ff : { 0.1, -0.1 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("realcc")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav lorealcc1=0 hirealcc1=0.1
            <region> sample=kick.wav lorealcc1=0 hirealcc1=0.1 lorealcc2=0.1 hirealcc2=0.2
            <region> sample=kick.wav lorealcc1=0.1 hirealcc1=-0.1
        )");
        synth.dispatchMessage(client, 0, "/region0/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/cc_range2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/cc_range1", "", nullptr);
        std::vector<std::string> expected {
            "/region0/cc_range1,ff : { 0, 1 }",
            "/region1/cc_range1,ff : { 0, 0.1 }",
            "/region2/cc_range1,ff : { 0, 0.1 }",
            "/region2/cc_range2,ff : { 0.1, 0.2 }",
            "/region3/cc_range1,ff : { 0.1, -0.1 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Last keyswitch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav sw_last=12
            <region> sample=kick.wav sw_last=c4
            <region> sample=kick.wav sw_lolast=14 sw_hilast=16
            <region> sample=kick.wav sw_lolast=c4 sw_hilast=b5
            <region> sample=kick.wav sw_last=-1
        )");
        synth.dispatchMessage(client, 0, "/region0/sw_last", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/sw_last", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/sw_last", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/sw_last", "", nullptr);
        // TODO: activate for the new region parser ; can handle note names
        // synth.dispatchMessage(client, 0, "/region4/sw_last", "", nullptr);
        // TODO: activate for the new region parser ; ignore the second value
        // synth.dispatchMessage(client, 0, "/region5/sw_last", "", nullptr);
        std::vector<std::string> expected {
            "/region0/sw_last,N : {  }",
            "/region1/sw_last,i : { 12 }",
            "/region2/sw_last,i : { 60 }",
            "/region3/sw_last,ii : { 14, 16 }",
            // "/region4/sw_last,ii : { 60, 83 }",
            // "/region5/sw_last,ii : { 0, 0 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("sw_lolast disables sw_last over the whole region")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav sw_last=12 sw_lolast=14 sw_last=16
        )");
        synth.dispatchMessage(client, 0, "/region0/sw_last", "", nullptr);
        std::vector<std::string> expected {
            "/region0/sw_last,ii : { 14, 14 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Keyswitch label")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sw_label=hello
    )");
    synth.dispatchMessage(client, 0, "/region0/sw_label", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sw_label", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sw_label,N : {  }",
        "/region1/sw_label,s : { hello }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Upswitch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sw_up=16
        <region> sample=kick.wav sw_up=-1
        <region> sample=kick.wav sw_up=128
        <region> sample=kick.wav sw_up=c4
        <region> sample=kick.wav sw_up=64 sw_up=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sw_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sw_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sw_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sw_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/sw_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/sw_up", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sw_up,N : {  }",
        "/region1/sw_up,i : { 16 }",
        "/region2/sw_up,N : {  }",
        "/region3/sw_up,N : {  }",
        "/region4/sw_up,i : { 60 }",
        "/region5/sw_up,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Downswitch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sw_down=16
        <region> sample=kick.wav sw_down=-1
        <region> sample=kick.wav sw_down=128
        <region> sample=kick.wav sw_down=c4
        <region> sample=kick.wav sw_down=64 sw_down=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sw_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sw_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sw_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sw_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/sw_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/sw_down", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sw_down,N : {  }",
        "/region1/sw_down,i : { 16 }",
        "/region2/sw_down,N : {  }",
        "/region3/sw_down,N : {  }",
        "/region4/sw_down,i : { 60 }",
        "/region5/sw_down,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Previous keyswitch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sw_previous=16
        <region> sample=kick.wav sw_previous=-1
        <region> sample=kick.wav sw_previous=128
        <region> sample=kick.wav sw_previous=c4
        <region> sample=kick.wav sw_previous=64 sw_previous=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sw_previous", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sw_previous", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sw_previous", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sw_previous", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/sw_previous", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/sw_previous", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sw_previous,N : {  }",
        "/region1/sw_previous,i : { 16 }",
        "/region2/sw_previous,N : {  }",
        "/region3/sw_previous,N : {  }",
        "/region4/sw_previous,i : { 60 }",
        "/region5/sw_previous,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Velocity override")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sw_vel=current
        <region> sample=kick.wav sw_vel=previous
        <region> sample=kick.wav sw_vel=previous sw_vel=current
    )");
    synth.dispatchMessage(client, 0, "/region0/sw_vel", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sw_vel", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sw_vel", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sw_vel", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sw_vel,s : { current }",
        "/region1/sw_vel,s : { current }",
        "/region2/sw_vel,s : { previous }",
        "/region3/sw_vel,s : { current }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Aftertouch range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lochanaft=34 hichanaft=60
        <region> sample=kick.wav lochanaft=-3 hichanaft=60
        <region> sample=kick.wav lochanaft=20 hichanaft=-1
        <region> sample=kick.wav lochanaft=20 hichanaft=10
    )");
    synth.dispatchMessage(client, 0, "/region0/chanaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/chanaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/chanaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/chanaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/chanaft_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/chanaft_range,ff : { 0, 1 }",
        "/region1/chanaft_range,ff : { 0.267717, 0.472441 }",
        "/region2/chanaft_range,ff : { -0.023622, 0.472441 }",
        "/region3/chanaft_range,ff : { 0.15748, -0.00787402 }",
        "/region4/chanaft_range,ff : { 0.15748, 0.0787402 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Polyaftertouch range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lopolyaft=34 hipolyaft=60
        <region> sample=kick.wav lopolyaft=-3 hipolyaft=60
        <region> sample=kick.wav lopolyaft=20 hipolyaft=-1
        <region> sample=kick.wav lopolyaft=20 hipolyaft=10
    )");
    synth.dispatchMessage(client, 0, "/region0/polyaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/polyaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/polyaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/polyaft_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/polyaft_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/polyaft_range,ff : { 0, 1 }",
        "/region1/polyaft_range,ff : { 0.267717, 0.472441 }",
        "/region2/polyaft_range,ff : { -0.023622, 0.472441 }",
        "/region3/polyaft_range,ff : { 0.15748, -0.00787402 }",
        "/region4/polyaft_range,ff : { 0.15748, 0.0787402 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] BPM range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lobpm=34.1 hibpm=60.2
        <region> sample=kick.wav lobpm=-3 hibpm=60
        <region> sample=kick.wav lobpm=20 hibpm=-1
        <region> sample=kick.wav lobpm=20 hibpm=10
    )");
    synth.dispatchMessage(client, 0, "/region0/bpm_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bpm_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bpm_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/bpm_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/bpm_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/bpm_range,ff : { 0, 500 }",
        "/region1/bpm_range,ff : { 34.1, 60.2 }",
        "/region2/bpm_range,ff : { -3, 60 }",
        "/region3/bpm_range,ff : { 20, -1 }",
        "/region4/bpm_range,ff : { 20, 10 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Rand range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav lorand=0.2 hirand=0.4
        <region> sample=kick.wav lorand=-0.1 hirand=0.4
        <region> sample=kick.wav lorand=0.2 hirand=-0.1
        <region> sample=kick.wav lorand=0.2 hirand=0.1
    )");
    synth.dispatchMessage(client, 0, "/region0/rand_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/rand_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/rand_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/rand_range", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/rand_range", "", nullptr);
    std::vector<std::string> expected {
        "/region0/rand_range,ff : { 0, 1 }",
        "/region1/rand_range,ff : { 0.2, 0.4 }",
        "/region2/rand_range,ff : { -0.1, 0.4 }",
        "/region3/rand_range,ff : { 0.2, -0.1 }",
        "/region4/rand_range,ff : { 0.2, 0.1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sequence length")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav seq_length=12
        <region> sample=kick.wav seq_length=-1
        <region> sample=kick.wav seq_length=12 seq_length=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/seq_length", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/seq_length", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/seq_length", "", nullptr);
    // TODO: activate for the new region parser ; ignore the second value
    // synth.dispatchMessage(client, 0, "/region3/seq_length", "", nullptr);
    std::vector<std::string> expected {
        "/region0/seq_length,h : { 1 }",
        "/region1/seq_length,h : { 12 }",
        "/region2/seq_length,h : { 1 }",
        // TODO: activate for the new region parser ; ignore the second value
        // "/region3/seq_length,f : { 12 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sequence position")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav seq_position=12
        <region> sample=kick.wav seq_position=-1
        <region> sample=kick.wav seq_position=12 seq_position=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/seq_position", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/seq_position", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/seq_position", "", nullptr);
    // TODO: activate for the new region parser ; ignore the second value
    // synth.dispatchMessage(client, 0, "/region3/seq_position", "", nullptr);
    std::vector<std::string> expected {
        "/region0/seq_position,h : { 1 }",
        "/region1/seq_position,h : { 12 }",
        "/region2/seq_position,h : { 1 }",
        // TODO: activate for the new region parser ; ignore the second value
        // "/region3/seq_position,f : { 12 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Trigger type")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav trigger=release
        <region> sample=kick.wav trigger=release_key
        <region> sample=kick.wav trigger=legato
        <region> sample=kick.wav trigger=first
        <region> sample=kick.wav trigger=nothing
        <region> sample=kick.wav trigger=release trigger=attack
    )");
    synth.dispatchMessage(client, 0, "/region0/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/trigger", "", nullptr);
    synth.dispatchMessage(client, 0, "/region6/trigger", "", nullptr);
    std::vector<std::string> expected {
        "/region0/trigger,s : { attack }",
        "/region1/trigger,s : { release }",
        "/region2/trigger,s : { release_key }",
        "/region3/trigger,s : { legato }",
        "/region4/trigger,s : { first }",
        "/region5/trigger,s : { attack }",
        "/region6/trigger,s : { attack }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Start on cc range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav on_locc1=15
        <region> sample=kick.wav on_hicc1=84
        <region> sample=kick.wav on_locc1=15 on_hicc1=84
        <region> sample=kick.wav on_lohdcc2=0.1
        <region> sample=kick.wav on_hihdcc2=0.4
        <region> sample=kick.wav on_lohdcc2=0.1 on_hihdcc2=0.4
    )");
    synth.dispatchMessage(client, 0, "/region0/start_cc_range1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/start_cc_range2", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/start_cc_range1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/start_cc_range1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/start_cc_range1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/start_cc_range2", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/start_cc_range2", "", nullptr);
    synth.dispatchMessage(client, 0, "/region6/start_cc_range2", "", nullptr);
    std::vector<std::string> expected {
        "/region0/start_cc_range1,N : {  }",
        "/region0/start_cc_range2,N : {  }",
        "/region1/start_cc_range1,ff : { 0.11811, 1 }",
        "/region2/start_cc_range1,ff : { 0, 0.661417 }",
        "/region3/start_cc_range1,ff : { 0.11811, 0.661417 }",
        "/region4/start_cc_range2,ff : { 0.1, 1 }",
        "/region5/start_cc_range2,ff : { 0, 0.4 }",
        "/region6/start_cc_range2,ff : { 0.1, 0.4 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Volume")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume=4.2
            <region> sample=kick.wav gain=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/volume", "", nullptr);
        // TODO: activate for the new region parser ; allow oob
        // synth.dispatchMessage(client, 0, "/region2/volume", "", nullptr);
        std::vector<std::string> expected {
            "/region0/volume,f : { 0 }",
            "/region1/volume,f : { 4.2 }",
            // "/region2/volume,f : { -200 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume_oncc42=4.2
            <region> sample=kick.wav gain_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/volume_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/volume_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/volume_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/volume_cc42,N : {  }",
            "/region1/volume_cc42,f : { 4.2 }",
            "/region2/volume_cc2,f : { -10 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav volume_stepcc42=4.2
            <region> sample=kick.wav volume_smoothcc42=4
            <region> sample=kick.wav volume_curvecc42=2
            <region> sample=kick.wav volume_stepcc42=-1
            <region> sample=kick.wav volume_smoothcc42=-4
            <region> sample=kick.wav volume_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/volume_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/volume_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/volume_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/volume_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/volume_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/volume_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/volume_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/volume_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/volume_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/volume_stepcc42,N : {  }",
            "/region0/volume_smoothcc42,N : {  }",
            "/region0/volume_curvecc42,N : {  }",
            "/region1/volume_stepcc42,f : { 4.2 }",
            "/region2/volume_smoothcc42,i : { 4 }",
            "/region3/volume_curvecc42,i : { 2 }",
            // "/region4/volume_stepcc42,N : {  }",
            // "/region5/volume_smoothcc42,N : {  }",
            // "/region6/volume_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params (with gain_)")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav gain_stepcc42=4.2
            <region> sample=kick.wav gain_smoothcc42=4
            <region> sample=kick.wav gain_curvecc42=2
            <region> sample=kick.wav gain_stepcc42=-1
            <region> sample=kick.wav gain_smoothcc42=-4
            <region> sample=kick.wav gain_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/volume_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/volume_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/volume_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/volume_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/volume_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/volume_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/volume_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/volume_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/volume_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/volume_stepcc42,N : {  }",
            "/region0/volume_smoothcc42,N : {  }",
            "/region0/volume_curvecc42,N : {  }",
            "/region1/volume_stepcc42,f : { 4.2 }",
            "/region2/volume_smoothcc42,i : { 4 }",
            "/region3/volume_curvecc42,i : { 2 }",
            // "/region4/volume_stepcc42,N : {  }",
            // "/region5/volume_smoothcc42,N : {  }",
            // "/region6/volume_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Pan")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan=4.2
            <region> sample=kick.wav pan=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/pan", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pan", "", nullptr);
        // TODO: activate for the new region parser ; accept oob
        // synth.dispatchMessage(client, 0, "/region2/pan", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pan,f : { 0 }",
            "/region1/pan,f : { 4.2 }",
            // TODO: activate for the new region parser ; accept oob
            // "/region2/pan,f : { -200 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan_oncc42=4.2
            <region> sample=kick.wav pan_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/pan_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pan_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pan_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pan_cc42,N : {  }",
            "/region1/pan_cc42,f : { 4.2 }",
            "/region2/pan_cc2,f : { -10 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pan_stepcc42=4.2
            <region> sample=kick.wav pan_smoothcc42=4
            <region> sample=kick.wav pan_curvecc42=2
            <region> sample=kick.wav pan_stepcc42=-1
            <region> sample=kick.wav pan_smoothcc42=-4
            <region> sample=kick.wav pan_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/pan_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pan_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pan_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pan_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pan_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/pan_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/pan_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/pan_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/pan_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pan_stepcc42,N : {  }",
            "/region0/pan_smoothcc42,N : {  }",
            "/region0/pan_curvecc42,N : {  }",
            "/region1/pan_stepcc42,f : { 4.2 }",
            "/region2/pan_smoothcc42,i : { 4 }",
            "/region3/pan_curvecc42,i : { 2 }",
            // "/region4/pan_stepcc42,N : {  }",
            // "/region5/pan_smoothcc42,N : {  }",
            // "/region6/pan_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Width")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width=4.2
            <region> sample=kick.wav width=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/width", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/width", "", nullptr);
        // TODO: activate for the new region parser ; accept oob
        // synth.dispatchMessage(client, 0, "/region2/width", "", nullptr);
        std::vector<std::string> expected {
            "/region0/width,f : { 100 }",
            "/region1/width,f : { 4.2 }",
            // TODO: activate for the new region parser ; accept oob
            // "/region2/width,f : { -200 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width_oncc42=4.2
            <region> sample=kick.wav width_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/width_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/width_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/width_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/width_cc42,N : {  }",
            "/region1/width_cc42,f : { 4.2 }",
            "/region2/width_cc2,f : { -10 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav width_stepcc42=4.2
            <region> sample=kick.wav width_smoothcc42=4
            <region> sample=kick.wav width_curvecc42=2
            <region> sample=kick.wav width_stepcc42=-1
            <region> sample=kick.wav width_smoothcc42=-4
            <region> sample=kick.wav width_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/width_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/width_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/width_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/width_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/width_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/width_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/width_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/width_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/width_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/width_stepcc42,N : {  }",
            "/region0/width_smoothcc42,N : {  }",
            "/region0/width_curvecc42,N : {  }",
            "/region1/width_stepcc42,f : { 4.2 }",
            "/region2/width_smoothcc42,i : { 4 }",
            "/region3/width_curvecc42,i : { 2 }",
            // "/region4/width_stepcc42,N : {  }",
            // "/region5/width_smoothcc42,N : {  }",
            // "/region6/width_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Position")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position=4.2
            <region> sample=kick.wav position=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/position", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/position", "", nullptr);
        // TODO: activate for the new region parser; accept oob
        // synth.dispatchMessage(client, 0, "/region2/position", "", nullptr);
        std::vector<std::string> expected {
            "/region0/position,f : { 0 }",
            "/region1/position,f : { 4.2 }",
            // TODO: activate for the new region parser; accept oob
            // "/region2/position,f : { -200 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position_oncc42=4.2
            <region> sample=kick.wav position_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/position_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/position_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/position_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/position_cc42,N : {  }",
            "/region1/position_cc42,f : { 4.2 }",
            "/region2/position_cc2,f : { -10 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav position_stepcc42=4.2
            <region> sample=kick.wav position_smoothcc42=4
            <region> sample=kick.wav position_curvecc42=2
            <region> sample=kick.wav position_stepcc42=-1
            <region> sample=kick.wav position_smoothcc42=-4
            <region> sample=kick.wav position_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/position_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/position_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/position_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/position_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/position_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/position_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/position_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/position_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/position_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/position_stepcc42,N : {  }",
            "/region0/position_smoothcc42,N : {  }",
            "/region0/position_curvecc42,N : {  }",
            "/region1/position_stepcc42,f : { 4.2 }",
            "/region2/position_smoothcc42,i : { 4 }",
            "/region3/position_curvecc42,i : { 2 }",
            // "/region4/position_stepcc42,N : {  }",
            // "/region5/position_smoothcc42,N : {  }",
            // "/region6/position_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Amplitude")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude=4.2
            <region> sample=kick.wav amplitude=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/amplitude", "", nullptr);
        // TODO: activate for the new region parser; ignore oob
        // synth.dispatchMessage(client, 0, "/region2/amplitude", "", nullptr);
        std::vector<std::string> expected {
            "/region0/amplitude,f : { 100 }",
            "/region1/amplitude,f : { 4.2 }",
            // "/region2/amplitude,f : { 100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude_oncc42=4.2
            <region> sample=kick.wav amplitude_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/amplitude_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/amplitude_cc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region2/amplitude_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/amplitude_cc42,N : {  }",
            "/region1/amplitude_cc42,f : { 4.2 }",
            // "/region2/amplitude_cc2,N : {  }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav amplitude_stepcc42=4.2
            <region> sample=kick.wav amplitude_smoothcc42=4
            <region> sample=kick.wav amplitude_curvecc42=2
            <region> sample=kick.wav amplitude_stepcc42=-1
            <region> sample=kick.wav amplitude_smoothcc42=-4
            <region> sample=kick.wav amplitude_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/amplitude_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/amplitude_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/amplitude_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/amplitude_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/amplitude_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/amplitude_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/amplitude_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/amplitude_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/amplitude_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/amplitude_stepcc42,N : {  }",
            "/region0/amplitude_smoothcc42,N : {  }",
            "/region0/amplitude_curvecc42,N : {  }",
            "/region1/amplitude_stepcc42,f : { 4.2 }",
            "/region2/amplitude_smoothcc42,i : { 4 }",
            "/region3/amplitude_curvecc42,i : { 2 }",
            // "/region4/amplitude_stepcc42,N : {  }",
            // "/region5/amplitude_smoothcc42,N : {  }",
            // "/region6/amplitude_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Amp Keycenter")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav amp_keycenter=40
        <region> sample=kick.wav amp_keycenter=-1
        <region> sample=kick.wav amp_keycenter=c3
    )");
    synth.dispatchMessage(client, 0, "/region0/amp_keycenter", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/amp_keycenter", "", nullptr);
    // TODO: activate for the new region parser ; ignore oob and parse note
    // synth.dispatchMessage(client, 0, "/region2/amp_keycenter", "", nullptr);
    // synth.dispatchMessage(client, 0, "/region3/amp_keycenter", "", nullptr);
    std::vector<std::string> expected {
        "/region0/amp_keycenter,i : { 60 }",
        "/region1/amp_keycenter,i : { 40 }",
        // "/region2/amp_keycenter,i : { 60 }",
        // "/region3/amp_keycenter,i : { 48 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Amp Keytrack")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav amp_keytrack=10.1
        <region> sample=kick.wav amp_keytrack=40
    )");
    synth.dispatchMessage(client, 0, "/region0/amp_keytrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/amp_keytrack", "", nullptr);
    // TODO: activate for the new region parser ; accept oob
    // synth.dispatchMessage(client, 0, "/region2/amp_keytrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/amp_keytrack,f : { 0 }",
        "/region1/amp_keytrack,f : { 10.1 }",
        // "/region2/amp_keytrack,f : { 40 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Amp Veltrack")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav amp_veltrack=10.1
        <region> sample=kick.wav amp_veltrack=-132
    )");
    synth.dispatchMessage(client, 0, "/region0/amp_veltrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/amp_veltrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/amp_veltrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/amp_veltrack,f : { 100 }",
        "/region1/amp_veltrack,f : { 10.1 }",
        "/region2/amp_veltrack,f : { -132 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Amp Random")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav amp_random=10.1
        <region> sample=kick.wav amp_random=-4
    )");
    synth.dispatchMessage(client, 0, "/region0/amp_random", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/amp_random", "", nullptr);
    // TODO: activate for the new region parser ; ignore oob
    // synth.dispatchMessage(client, 0, "/region2/amp_random", "", nullptr);
    std::vector<std::string> expected {
        "/region0/amp_random,f : { 0 }",
        "/region1/amp_random,f : { 10.1 }",
        // "/region2/amp_random,f : { 0 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Crossfade key range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Xfin")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_lokey=10 xfin_hikey=40
            <region> sample=kick.wav xfin_lokey=c4 xfin_hikey=b5
            <region> sample=kick.wav xfin_lokey=-10 xfin_hikey=40
            <region> sample=kick.wav xfin_lokey=10 xfin_hikey=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfin_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfin_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfin_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfin_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region4/xfin_key_range", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfin_key_range,ii : { 0, 0 }",
            "/region1/xfin_key_range,ii : { 10, 40 }",
            "/region2/xfin_key_range,ii : { 60, 83 }",
            "/region3/xfin_key_range,ii : { 0, 40 }",
            "/region4/xfin_key_range,ii : { 10, 0 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Xfout")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_lokey=10 xfout_hikey=40
            <region> sample=kick.wav xfout_lokey=c4 xfout_hikey=b5
            <region> sample=kick.wav xfout_lokey=-10 xfout_hikey=40
            <region> sample=kick.wav xfout_lokey=10 xfout_hikey=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfout_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfout_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfout_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfout_key_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region4/xfout_key_range", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfout_key_range,ii : { 127, 127 }",
            "/region1/xfout_key_range,ii : { 10, 40 }",
            "/region2/xfout_key_range,ii : { 60, 83 }",
            "/region3/xfout_key_range,ii : { 127, 40 }",
            "/region4/xfout_key_range,ii : { 10, 127 }",
        };
        REQUIRE(messageList == expected);
    }
}


TEST_CASE("[Values] Crossfade velocity range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Xfin")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_lovel=10 xfin_hivel=40
            <region> sample=kick.wav xfin_lovel=-10 xfin_hivel=40
            <region> sample=kick.wav xfin_lovel=10 xfin_hivel=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfin_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfin_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfin_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfin_vel_range", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfin_vel_range,ff : { 0, 0 }",
            "/region1/xfin_vel_range,ff : { 0.0787402, 0.314961 }",
            "/region2/xfin_vel_range,ff : { -0.0787402, 0.314961 }",
            "/region3/xfin_vel_range,ff : { 0.0787402, 1.10236 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Xfout")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_lovel=10 xfout_hivel=40
            <region> sample=kick.wav xfout_lovel=-10 xfout_hivel=40
            <region> sample=kick.wav xfout_lovel=10 xfout_hivel=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfout_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfout_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfout_vel_range", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfout_vel_range", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfout_vel_range,ff : { 1, 1 }",
            "/region1/xfout_vel_range,ff : { 0.0787402, 0.314961 }",
            "/region2/xfout_vel_range,ff : { -0.0787402, 0.314961 }",
            "/region3/xfout_vel_range,ff : { 0.0787402, 1.10236 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Crossfade curves")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Key")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_keycurve=gain
            <region> sample=kick.wav xf_keycurve=something
            <region> sample=kick.wav xf_keycurve=gain xf_keycurve=power
        )");
        synth.dispatchMessage(client, 0, "/region0/xf_keycurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xf_keycurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xf_keycurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xf_keycurve", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xf_keycurve,s : { power }",
            "/region1/xf_keycurve,s : { gain }",
            "/region2/xf_keycurve,s : { power }",
            "/region3/xf_keycurve,s : { power }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Velocity")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_velcurve=gain
            <region> sample=kick.wav xf_velcurve=something
            <region> sample=kick.wav xf_velcurve=gain xf_velcurve=power
        )");
        synth.dispatchMessage(client, 0, "/region0/xf_velcurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xf_velcurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xf_velcurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xf_velcurve", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xf_velcurve,s : { power }",
            "/region1/xf_velcurve,s : { gain }",
            "/region2/xf_velcurve,s : { power }",
            "/region3/xf_velcurve,s : { power }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xf_cccurve=gain
            <region> sample=kick.wav xf_cccurve=something
            <region> sample=kick.wav xf_cccurve=gain xf_cccurve=power
        )");
        synth.dispatchMessage(client, 0, "/region0/xf_cccurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xf_cccurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xf_cccurve", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xf_cccurve", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xf_cccurve,s : { power }",
            "/region1/xf_cccurve,s : { gain }",
            "/region2/xf_cccurve,s : { power }",
            "/region3/xf_cccurve,s : { power }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Crossfade CC range")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Xfin")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfin_locc4=10 xfin_hicc4=40
            <region> sample=kick.wav xfin_locc4=-10 xfin_hicc4=40
            <region> sample=kick.wav xfin_locc4=10 xfin_hicc4=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfin_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfin_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfin_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfin_cc_range4", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfin_cc_range4,N : {  }",
            "/region1/xfin_cc_range4,ff : { 0.0787402, 0.314961 }",
            "/region2/xfin_cc_range4,ff : { -0.0787402, 0.314961 }",
            "/region3/xfin_cc_range4,ff : { 0.0787402, 1.10236 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Xfout")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav xfout_locc4=10 xfout_hicc4=40
            <region> sample=kick.wav xfout_locc4=-10 xfout_hicc4=40
            <region> sample=kick.wav xfout_locc4=10 xfout_hicc4=140
        )");
        synth.dispatchMessage(client, 0, "/region0/xfout_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/xfout_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/xfout_cc_range4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/xfout_cc_range4", "", nullptr);
        std::vector<std::string> expected {
            "/region0/xfout_cc_range4,N : {  }",
            "/region1/xfout_cc_range4,ff : { 0.0787402, 0.314961 }",
            "/region2/xfout_cc_range4,ff : { -0.0787402, 0.314961 }",
            "/region3/xfout_cc_range4,ff : { 0.0787402, 1.10236 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Global volumes and amplitudes")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Volumes")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <global> global_volume=4.4
            <master> master_volume=5.5
            <group> group_volume=6.6
            <region> sample=kick.wav
        )");
        synth.dispatchMessage(client, 0, "/region0/global_volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/master_volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/group_volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/global_volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/master_volume", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/group_volume", "", nullptr);
        std::vector<std::string> expected {
            "/region0/global_volume,f : { 0 }",
            "/region0/master_volume,f : { 0 }",
            "/region0/group_volume,f : { 0 }",
            "/region1/global_volume,f : { 4.4 }",
            "/region1/master_volume,f : { 5.5 }",
            "/region1/group_volume,f : { 6.6 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Amplitudes")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <global> global_amplitude=4.4
            <master> master_amplitude=5.5
            <group> group_amplitude=6.6
            <region> sample=kick.wav
        )");
        synth.dispatchMessage(client, 0, "/region0/global_amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/master_amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/group_amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/global_amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/master_amplitude", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/group_amplitude", "", nullptr);
        std::vector<std::string> expected {
            "/region0/global_amplitude,f : { 100 }",
            "/region0/master_amplitude,f : { 100 }",
            "/region0/group_amplitude,f : { 100 }",
            "/region1/global_amplitude,f : { 4.4 }",
            "/region1/master_amplitude,f : { 5.5 }",
            "/region1/group_amplitude,f : { 6.6 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Pitch Keytrack")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav pitch_keytrack=1000
        <region> sample=kick.wav pitch_keytrack=-100
    )");
    synth.dispatchMessage(client, 0, "/region0/pitch_keytrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/pitch_keytrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/pitch_keytrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/pitch_keytrack,i : { 100 }",
        "/region1/pitch_keytrack,i : { 1000 }",
        "/region2/pitch_keytrack,i : { -100 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Pitch Veltrack")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav pitch_veltrack=10
        <region> sample=kick.wav pitch_veltrack=-132
    )");
    synth.dispatchMessage(client, 0, "/region0/pitch_veltrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/pitch_veltrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/pitch_veltrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/pitch_veltrack,i : { 0 }",
        "/region1/pitch_veltrack,i : { 10 }",
        "/region2/pitch_veltrack,i : { -132 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Pitch Random")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav pitch_random=10
        <region> sample=kick.wav pitch_random=-4
    )");
    synth.dispatchMessage(client, 0, "/region0/pitch_random", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/pitch_random", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/pitch_random", "", nullptr);
    std::vector<std::string> expected {
        "/region0/pitch_random,f : { 0 }",
        "/region1/pitch_random,f : { 10 }",
        "/region2/pitch_random,f : { -4 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Transpose")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav transpose=10
        <region> sample=kick.wav transpose=-4
        <region> sample=kick.wav transpose=-400
        <region> sample=kick.wav transpose=400
    )");
    synth.dispatchMessage(client, 0, "/region0/transpose", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/transpose", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/transpose", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/transpose", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/transpose", "", nullptr);
    std::vector<std::string> expected {
        "/region0/transpose,i : { 0 }",
        "/region1/transpose,i : { 10 }",
        "/region2/transpose,i : { -4 }",
        "/region3/transpose,i : { -400 }",
        "/region4/transpose,i : { 400 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Pitch/Tune")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch=4.2
            <region> sample=kick.wav tune=-200
        )");
        synth.dispatchMessage(client, 0, "/region0/pitch", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pitch", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pitch", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pitch,f : { 0 }",
            "/region1/pitch,f : { 4.2 }",
            "/region2/pitch,f : { -200 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Depth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_oncc42=4.2
            <region> sample=kick.wav pitch_oncc2=-10
        )");
        synth.dispatchMessage(client, 0, "/region0/pitch_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pitch_cc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pitch_cc2", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pitch_cc42,N : {  }",
            "/region1/pitch_cc42,f : { 4.2 }",
            "/region2/pitch_cc2,f : { -10 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_stepcc42=4.2
            <region> sample=kick.wav pitch_smoothcc42=4
            <region> sample=kick.wav pitch_curvecc42=2
            <region> sample=kick.wav pitch_stepcc42=-1
            <region> sample=kick.wav pitch_smoothcc42=-4
            <region> sample=kick.wav pitch_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/pitch_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pitch_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pitch_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pitch_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pitch_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/pitch_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/pitch_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/pitch_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/pitch_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pitch_stepcc42,N : {  }",
            "/region0/pitch_smoothcc42,N : {  }",
            "/region0/pitch_curvecc42,N : {  }",
            "/region1/pitch_stepcc42,f : { 4.2 }",
            "/region2/pitch_smoothcc42,i : { 4 }",
            "/region3/pitch_curvecc42,i : { 2 }",
            // "/region4/pitch_stepcc42,N : {  }",
            // "/region5/pitch_smoothcc42,N : {  }",
            // "/region6/pitch_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("CC Params (with pitch_)")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav pitch_stepcc42=4.2
            <region> sample=kick.wav pitch_smoothcc42=4
            <region> sample=kick.wav pitch_curvecc42=2
            <region> sample=kick.wav pitch_stepcc42=-1
            <region> sample=kick.wav pitch_smoothcc42=-4
            <region> sample=kick.wav pitch_curvecc42=300
        )");
        synth.dispatchMessage(client, 0, "/region0/pitch_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pitch_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/pitch_curvecc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/pitch_stepcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/pitch_smoothcc42", "", nullptr);
        synth.dispatchMessage(client, 0, "/region3/pitch_curvecc42", "", nullptr);
        // TODO: activate for the new region parser ; ignore oob
        // synth.dispatchMessage(client, 0, "/region4/pitch_stepcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region5/pitch_smoothcc42", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region6/pitch_curvecc42", "", nullptr);
        std::vector<std::string> expected {
            "/region0/pitch_stepcc42,N : {  }",
            "/region0/pitch_smoothcc42,N : {  }",
            "/region0/pitch_curvecc42,N : {  }",
            "/region1/pitch_stepcc42,f : { 4.2 }",
            "/region2/pitch_smoothcc42,i : { 4 }",
            "/region3/pitch_curvecc42,i : { 2 }",
            // "/region4/pitch_stepcc42,N : {  }",
            // "/region5/pitch_smoothcc42,N : {  }",
            // "/region6/pitch_curvecc42,N : {  }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Bend behavior")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav bend_up=100 bend_down=-400 bend_step=10 bend_smooth=10
        <region> sample=kick.wav bend_up=-100 bend_down=400 bend_step=-10 bend_smooth=-10
    )");
    synth.dispatchMessage(client, 0, "/region0/bend_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/bend_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/bend_step", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/bend_smooth", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bend_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bend_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bend_step", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/bend_smooth", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bend_up", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bend_down", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bend_step", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/bend_smooth", "", nullptr);
    std::vector<std::string> expected {
        "/region0/bend_up,f : { 200 }",
        "/region0/bend_down,f : { -200 }",
        "/region0/bend_step,f : { 1 }",
        "/region0/bend_smooth,i : { 0 }",
        "/region1/bend_up,f : { 100 }",
        "/region1/bend_down,f : { -400 }",
        "/region1/bend_step,f : { 10 }",
        "/region1/bend_smooth,i : { 10 }",
        "/region2/bend_up,f : { -100 }",
        "/region2/bend_down,f : { 400 }",
        "/region2/bend_step,f : { 1 }",
        "/region2/bend_smooth,i : { 0 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] ampeg")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
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
        synth.dispatchMessage(client, 0, "/region0/ampeg_attack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_decay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_hold", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_release", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_start", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_sustain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_depth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_attack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_decay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_hold", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_release", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_start", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_sustain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_depth", "", nullptr);
        // TODO after new parser : ignore oob
        // synth.dispatchMessage(client, 0, "/region2/ampeg_attack", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_delay", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_decay", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_hold", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_release", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_start", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_sustain", "", nullptr);
        // synth.dispatchMessage(client, 0, "/region2/ampeg_depth", "", nullptr);
        std::vector<std::string> expected {
            "/region0/ampeg_attack,f : { 0 }",
            "/region0/ampeg_delay,f : { 0 }",
            "/region0/ampeg_decay,f : { 0 }",
            "/region0/ampeg_hold,f : { 0 }",
            "/region0/ampeg_release,f : { 0.001 }",
            "/region0/ampeg_start,f : { 0 }",
            "/region0/ampeg_sustain,f : { 100 }",
            "/region0/ampeg_depth,f : { 0 }",
            "/region1/ampeg_attack,f : { 1 }",
            "/region1/ampeg_delay,f : { 2 }",
            "/region1/ampeg_decay,f : { 3 }",
            "/region1/ampeg_hold,f : { 4 }",
            "/region1/ampeg_release,f : { 5 }",
            "/region1/ampeg_start,f : { 6 }",
            "/region1/ampeg_sustain,f : { 7 }",
            "/region1/ampeg_depth,f : { 0 }",
            // "/region2/ampeg_attack,f : { 0 }",
            // "/region2/ampeg_delay,f : { 0 }",
            // "/region2/ampeg_decay,f : { 0 }",
            // "/region2/ampeg_hold,f : { 0 }",
            // "/region2/ampeg_release,f : { 0.001 }",
            // "/region2/ampeg_start,f : { 0 }",
            // "/region2/ampeg_sustain,f : { 100 }",
            // "/region2/ampeg_depth,f : { 0 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Velocity")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
            <region> sample=kick.wav
                ampeg_vel2attack=1 ampeg_vel2delay=2 ampeg_vel2decay=3
                ampeg_vel2hold=4 ampeg_vel2release=5
                ampeg_vel2sustain=7 ampeg_vel2depth=8
        )");
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2attack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2decay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2hold", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2release", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2sustain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_vel2depth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2attack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2delay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2decay", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2hold", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2release", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2sustain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/ampeg_vel2depth", "", nullptr);
        std::vector<std::string> expected {
            "/region0/ampeg_vel2attack,f : { 0 }",
            "/region0/ampeg_vel2delay,f : { 0 }",
            "/region0/ampeg_vel2decay,f : { 0 }",
            "/region0/ampeg_vel2hold,f : { 0 }",
            "/region0/ampeg_vel2release,f : { 0 }",
            "/region0/ampeg_vel2sustain,f : { 0 }",
            "/region0/ampeg_vel2depth,f : { 0 }",
            "/region1/ampeg_vel2attack,f : { 1 }",
            "/region1/ampeg_vel2delay,f : { 2 }",
            "/region1/ampeg_vel2decay,f : { 3 }",
            "/region1/ampeg_vel2hold,f : { 4 }",
            "/region1/ampeg_vel2release,f : { 5 }",
            "/region1/ampeg_vel2sustain,f : { 7 }",
            "/region1/ampeg_vel2depth,f : { 0 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Note polyphony")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav note_polyphony=10
        <region> sample=kick.wav note_polyphony=-4
        <region> sample=kick.wav note_polyphony=10 note_polyphony=-4
    )");
    synth.dispatchMessage(client, 0, "/region0/note_polyphony", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/note_polyphony", "", nullptr);
    // TODO: activate for the new region parser ; ignore oob
    // synth.dispatchMessage(client, 0, "/region2/note_polyphony", "", nullptr);
    // synth.dispatchMessage(client, 0, "/region3/note_polyphony", "", nullptr);
    std::vector<std::string> expected {
        "/region0/note_polyphony,N : {  }",
        "/region1/note_polyphony,i : { 10 }",
        // "/region2/note_polyphony,N : {  }",
        // "/region3/note_polyphony,i : { 10 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Self-mask")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav note_selfmask=off
        <region> sample=kick.wav note_selfmask=off note_selfmask=on
        <region> sample=kick.wav note_selfmask=off note_selfmask=garbage
    )");
    synth.dispatchMessage(client, 0, "/region0/note_selfmask", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/note_selfmask", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/note_selfmask", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/note_selfmask", "", nullptr);
    std::vector<std::string> expected {
        "/region0/note_selfmask,T : {  }",
        "/region1/note_selfmask,F : {  }",
        "/region2/note_selfmask,T : {  }",
        "/region3/note_selfmask,T : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] RT dead")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav rt_dead=on
        <region> sample=kick.wav rt_dead=on rt_dead=off
        <region> sample=kick.wav rt_dead=on rt_dead=garbage
    )");
    synth.dispatchMessage(client, 0, "/region0/rt_dead", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/rt_dead", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/rt_dead", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/rt_dead", "", nullptr);
    std::vector<std::string> expected {
        "/region0/rt_dead,F : {  }",
        "/region1/rt_dead,T : {  }",
        "/region2/rt_dead,F : {  }",
        "/region3/rt_dead,F : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sustain switch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sustain_sw=off
        <region> sample=kick.wav sustain_sw=off sustain_sw=on
        <region> sample=kick.wav sustain_sw=off sustain_sw=garbage
    )");
    synth.dispatchMessage(client, 0, "/region0/sustain_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sustain_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sustain_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sustain_sw", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sustain_sw,T : {  }",
        "/region1/sustain_sw,F : {  }",
        "/region2/sustain_sw,T : {  }",
        "/region3/sustain_sw,T : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sostenuto switch")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sostenuto_sw=off
        <region> sample=kick.wav sostenuto_sw=off sostenuto_sw=on
        <region> sample=kick.wav sostenuto_sw=off sostenuto_sw=garbage
    )");
    synth.dispatchMessage(client, 0, "/region0/sostenuto_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sostenuto_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sostenuto_sw", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/sostenuto_sw", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sostenuto_sw,T : {  }",
        "/region1/sostenuto_sw,F : {  }",
        "/region2/sostenuto_sw,T : {  }",
        "/region3/sostenuto_sw,T : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sustain CC")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sustain_cc=10
        <region> sample=kick.wav sustain_cc=20 sustain_cc=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sustain_cc", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sustain_cc", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sustain_cc", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sustain_cc,i : { 64 }",
        "/region1/sustain_cc,i : { 10 }",
        "/region2/sustain_cc,i : { 64 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sustain low")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sustain_lo=10
        <region> sample=kick.wav sustain_lo=10 sustain_lo=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sustain_lo", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sustain_lo", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sustain_lo", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sustain_lo,f : { 0.00787402 }",
        "/region1/sustain_lo,f : { 0.0787402 }",
        "/region2/sustain_lo,f : { -0.00787402 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sostenuto CC")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sostenuto_cc=10
        <region> sample=kick.wav sostenuto_cc=20 sostenuto_cc=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sostenuto_cc", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sostenuto_cc", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sostenuto_cc", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sostenuto_cc,i : { 66 }",
        "/region1/sostenuto_cc,i : { 10 }",
        "/region2/sostenuto_cc,i : { 66 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Sostenuto low")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav sostenuto_lo=10
        <region> sample=kick.wav sostenuto_lo=10 sostenuto_lo=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/sostenuto_lo", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/sostenuto_lo", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/sostenuto_lo", "", nullptr);
    std::vector<std::string> expected {
        "/region0/sostenuto_lo,f : { 0.00787402 }",
        "/region1/sostenuto_lo,f : { 0.0787402 }",
        "/region2/sostenuto_lo,f : { -0.00787402 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Oscillator phase")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav oscillator_phase=0.1
        <region> sample=kick.wav oscillator_phase=1.1
        <region> sample=kick.wav oscillator_phase=-1.2
    )");
    synth.dispatchMessage(client, 0, "/region0/oscillator_phase", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/oscillator_phase", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/oscillator_phase", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/oscillator_phase", "", nullptr);
    std::vector<std::string> expected {
        "/region0/oscillator_phase,f : { 0 }",
        "/region1/oscillator_phase,f : { 0.1 }",
        "/region2/oscillator_phase,f : { 0.1 }",
        "/region3/oscillator_phase,f : { -1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Oscillator quality")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav oscillator_quality=2
        <region> sample=kick.wav oscillator_quality=0 oscillator_quality=-2
    )");
    synth.dispatchMessage(client, 0, "/region0/oscillator_quality", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/oscillator_quality", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/oscillator_quality", "", nullptr);
    std::vector<std::string> expected {
        "/region0/oscillator_quality,N : {  }",
        "/region1/oscillator_quality,i : { 2 }",
        "/region2/oscillator_quality,N : {  }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Oscillator mode/multi")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav oscillator_mode=2
        <region> sample=kick.wav oscillator_mode=1 oscillator_mode=-2
        <region> sample=kick.wav oscillator_multi=9
        <region> sample=kick.wav oscillator_multi=-2
    )");
    synth.dispatchMessage(client, 0, "/region0/oscillator_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/oscillator_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/oscillator_mode", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/oscillator_multi", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/oscillator_multi", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/oscillator_multi", "", nullptr);
    std::vector<std::string> expected {
        "/region0/oscillator_mode,i : { 0 }",
        "/region1/oscillator_mode,i : { 2 }",
        "/region2/oscillator_mode,i : { 0 }",
        "/region0/oscillator_multi,i : { 1 }",
        "/region3/oscillator_multi,i : { 9 }",
        "/region4/oscillator_multi,i : { 1 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Oscillator detune/mod depth")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav oscillator_detune=9.2
        <region> sample=kick.wav oscillator_detune=-1200.2
        <region> sample=kick.wav oscillator_mod_depth=1564.75
        <region> sample=kick.wav oscillator_mod_depth=-2.2
    )");
    synth.dispatchMessage(client, 0, "/region0/oscillator_detune", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/oscillator_detune", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/oscillator_detune", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/oscillator_mod_depth", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/oscillator_mod_depth", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/oscillator_mod_depth", "", nullptr);
    std::vector<std::string> expected {
        "/region0/oscillator_detune,f : { 0 }",
        "/region1/oscillator_detune,f : { 9.2 }",
        "/region2/oscillator_detune,f : { -1200.2 }",
        "/region0/oscillator_mod_depth,f : { 0 }",
        "/region3/oscillator_mod_depth,f : { 1564.75 }",
        "/region4/oscillator_mod_depth,f : { -2.2 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Effect sends")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav effect1=10
        <region> sample=kick.wav effect2=50.4
        <region> sample=kick.wav effect1=-1
    )");
    synth.dispatchMessage(client, 0, "/region0/effect1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/effect1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/effect1", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/effect2", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/effect1", "", nullptr);
    std::vector<std::string> expected {
        // No reply to the first question
        "/region1/effect1,f : { 10 }",
        "/region2/effect1,f : { 0 }",
        "/region2/effect2,f : { 50.4 }",
        // No reply to the last question
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Support floating point for int values")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav offset=1042.5
        <region> sample=kick.wav pitch_keytrack=-2.1
    )");
    synth.dispatchMessage(client, 0, "/region0/offset", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/pitch_keytrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/offset,h : { 1042 }",
        "/region1/pitch_keytrack,i : { -2 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] ampeg CC")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
        )");
        synth.dispatchMessage(client, 0, "/region0/ampeg_attack_cc1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_delay_cc2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_decay_cc3", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_hold_cc4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_release_cc5", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_start_cc6", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_sustain_cc7", "", nullptr);
        std::vector<std::string> expected {
            "/region0/ampeg_attack_cc1,f : { 0 }",
            "/region0/ampeg_delay_cc2,f : { 0 }",
            "/region0/ampeg_decay_cc3,f : { 0 }",
            "/region0/ampeg_hold_cc4,f : { 0 }",
            "/region0/ampeg_release_cc5,f : { 0 }",
            "/region0/ampeg_start_cc6,f : { 0 }",
            "/region0/ampeg_sustain_cc7,f : { 0 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Positive values")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
                ampeg_attack_oncc1=1 ampeg_delay_oncc2=2 ampeg_decay_oncc3=3
                ampeg_hold_oncc4=4 ampeg_release_oncc5=5 ampeg_start_oncc6=6
                ampeg_sustain_oncc7=7
        )");
        synth.dispatchMessage(client, 0, "/region0/ampeg_attack_cc1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_delay_cc2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_decay_cc3", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_hold_cc4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_release_cc5", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_start_cc6", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_sustain_cc7", "", nullptr);
        std::vector<std::string> expected {
            "/region0/ampeg_attack_cc1,f : { 1 }",
            "/region0/ampeg_delay_cc2,f : { 2 }",
            "/region0/ampeg_decay_cc3,f : { 3 }",
            "/region0/ampeg_hold_cc4,f : { 4 }",
            "/region0/ampeg_release_cc5,f : { 5 }",
            "/region0/ampeg_start_cc6,f : { 6 }",
            "/region0/ampeg_sustain_cc7,f : { 7 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Basic")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav
                ampeg_attack_cc1=-1 ampeg_delay_cc2=-2 ampeg_decay_cc3=-3
                ampeg_hold_cc4=-4 ampeg_release_cc5=-5 ampeg_start_cc6=-6
                ampeg_sustain_cc7=-7
        )");
        synth.dispatchMessage(client, 0, "/region0/ampeg_attack_cc1", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_delay_cc2", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_decay_cc3", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_hold_cc4", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_release_cc5", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_start_cc6", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/ampeg_sustain_cc7", "", nullptr);
        std::vector<std::string> expected {
            "/region0/ampeg_attack_cc1,f : { -1 }",
            "/region0/ampeg_delay_cc2,f : { -2 }",
            "/region0/ampeg_decay_cc3,f : { -3 }",
            "/region0/ampeg_hold_cc4,f : { -4 }",
            "/region0/ampeg_release_cc5,f : { -5 }",
            "/region0/ampeg_start_cc6,f : { -6 }",
            "/region0/ampeg_sustain_cc7,f : { -7 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Filter stacking and cutoffs")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav cutoff=50
        <region> sample=kick.wav cutoff2=500
    )");

    SECTION("Test first region")
    {
        synth.dispatchMessage(client, 0, "/region0/filter0/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/filter1/type", "", nullptr);
        std::vector<std::string> expected {
            // No filters
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Test second region")
    {
        synth.dispatchMessage(client, 0, "/region1/filter0/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter1/type", "", nullptr);
        std::vector<std::string> expected {
            "/region1/filter0/cutoff,f : { 50 }",
            "/region1/filter0/gain,f : { 0 }",
            "/region1/filter0/resonance,f : { 0 }",
            "/region1/filter0/keycenter,i : { 60 }",
            "/region1/filter0/keytrack,i : { 0 }",
            "/region1/filter0/veltrack,i : { 0 }",
            "/region1/filter0/type,s : { lpf_2p }",
            // No second filter
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Test third region")
    {
        synth.dispatchMessage(client, 0, "/region2/filter0/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/resonance", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/keytrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/veltrack", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter1/type", "", nullptr);
        std::vector<std::string> expected {
            // The first filter is default-filled
            "/region2/filter0/cutoff,f : { 0 }",
            "/region2/filter0/gain,f : { 0 }",
            "/region2/filter0/resonance,f : { 0 }",
            "/region2/filter0/keycenter,i : { 60 }",
            "/region2/filter0/keytrack,i : { 0 }",
            "/region2/filter0/veltrack,i : { 0 }",
            "/region2/filter0/type,s : { lpf_2p }",
            "/region2/filter1/cutoff,f : { 500 }",
            "/region2/filter1/gain,f : { 0 }",
            "/region2/filter1/resonance,f : { 0 }",
            "/region2/filter1/keycenter,i : { 60 }",
            "/region2/filter1/keytrack,i : { 0 }",
            "/region2/filter1/veltrack,i : { 0 }",
            "/region2/filter1/type,s : { lpf_2p }",
            // No second filter
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] Filter types")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
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

    synth.dispatchMessage(client, 0, "/region0/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region4/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region5/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region6/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region7/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region8/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region9/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region10/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region11/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region12/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region13/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region14/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region15/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region16/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region17/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region18/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region19/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region20/filter0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region21/filter1/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region22/filter1/type", "", nullptr);
    std::vector<std::string> expected {
        "/region0/filter0/type,s : { lpf_1p }",
        "/region1/filter0/type,s : { hpf_1p }",
        "/region2/filter0/type,s : { lpf_2p }",
        "/region3/filter0/type,s : { hpf_2p }",
        "/region4/filter0/type,s : { bpf_2p }",
        "/region5/filter0/type,s : { brf_2p }",
        "/region6/filter0/type,s : { bpf_1p }",
        "/region7/filter0/type,s : { brf_1p }",
        "/region8/filter0/type,s : { apf_1p }",
        "/region9/filter0/type,s : { lpf_2p_sv }",
        "/region10/filter0/type,s : { hpf_2p_sv }",
        "/region11/filter0/type,s : { bpf_2p_sv }",
        "/region12/filter0/type,s : { brf_2p_sv }",
        "/region13/filter0/type,s : { lpf_4p }",
        "/region14/filter0/type,s : { hpf_4p }",
        "/region15/filter0/type,s : { lpf_6p }",
        "/region16/filter0/type,s : { hpf_6p }",
        "/region17/filter0/type,s : { pink }",
        "/region18/filter0/type,s : { lsh }",
        "/region19/filter0/type,s : { hsh }",
        "/region20/filter0/type,s : { peq }",
        "/region21/filter1/type,s : { peq }",
        "/region22/filter1/type,s : { none }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Filter dispatching")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
            cutoff3=50 resonance2=3 fil2_gain=-5 fil3_keytrack=100
            fil_gain=5 fil1_gain=-5 fil2_veltrack=-100
    )");

    synth.dispatchMessage(client, 0, "/region0/filter2/cutoff", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/filter1/resonance", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/filter1/gain", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/filter2/keytrack", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/filter0/gain", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/filter1/veltrack", "", nullptr);
    std::vector<std::string> expected {
        "/region0/filter2/cutoff,f : { 50 }",
        "/region0/filter1/resonance,f : { 3 }",
        "/region0/filter1/gain,f : { -5 }",
        "/region0/filter2/keytrack,i : { 100 }",
        "/region0/filter0/gain,f : { -5 }",
        "/region0/filter1/veltrack,i : { -100 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] Filter value bounds")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Cutoff")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav cutoff=100000
            <region> sample=kick.wav cutoff=50 cutoff=-100
        )");
        synth.dispatchMessage(client, 0, "/region0/filter0/cutoff", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/cutoff", "", nullptr);
        std::vector<std::string> expected {
            "/region0/filter0/cutoff,f : { 100000 }",
            "/region1/filter0/cutoff,f : { -100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Cutoff")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav resonance=5 resonance=-5
        )");
        synth.dispatchMessage(client, 0, "/region0/filter0/resonance", "", nullptr);
        std::vector<std::string> expected {
            "/region0/filter0/resonance,f : { -5 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Keycenter")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav fil_keycenter=40
            <region> sample=kick.wav fil_keycenter=40 fil_keycenter=1000
            <region> sample=kick.wav fil_keycenter=c3
        )");
        synth.dispatchMessage(client, 0, "/region0/filter0/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/filter0/keycenter", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/filter0/keycenter", "", nullptr);
        std::vector<std::string> expected {
            "/region0/filter0/keycenter,i : { 40 }",
            "/region1/filter0/keycenter,i : { 60 }",
            "/region2/filter0/keycenter,i : { 48 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] EQ stacking and gains")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
        <region> sample=kick.wav eq1_gain=3
        <region> sample=kick.wav eq4_gain=6
    )");

    SECTION("Test first region")
    {
        synth.dispatchMessage(client, 0, "/region0/eq0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq0/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq0/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq0/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq0/vel2freq", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region0/eq1/vel2freq", "", nullptr);
        std::vector<std::string> expected {
            // No eqs
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Test second region")
    {
        synth.dispatchMessage(client, 0, "/region1/eq0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/vel2freq", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq1/vel2freq", "", nullptr);
        std::vector<std::string> expected {
            "/region1/eq0/gain,f : { 3 }",
            "/region1/eq0/type,s : { peak }",
            "/region1/eq0/bandwidth,f : { 1 }",
            "/region1/eq0/frequency,f : { 50 }",
            "/region1/eq0/vel2gain,f : { 0 }",
            "/region1/eq0/vel2freq,f : { 0 }",
            // No second eq
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Test third region")
    {
        synth.dispatchMessage(client, 0, "/region2/eq0/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq0/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq0/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq0/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq0/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq0/vel2freq", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/type", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/bandwidth", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/vel2gain", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq3/vel2freq", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq1/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region2/eq2/frequency", "", nullptr);
        std::vector<std::string> expected {
            // The first eq is default-filled
            "/region2/eq0/gain,f : { 0 }",
            "/region2/eq0/type,s : { peak }",
            "/region2/eq0/bandwidth,f : { 1 }",
            "/region2/eq0/frequency,f : { 50 }",
            "/region2/eq0/vel2gain,f : { 0 }",
            "/region2/eq0/vel2freq,f : { 0 }",
            "/region2/eq3/gain,f : { 6 }",
            "/region2/eq3/type,s : { peak }",
            "/region2/eq3/bandwidth,f : { 1 }",
            "/region2/eq3/frequency,f : { 0 }",
            "/region2/eq3/vel2gain,f : { 0 }",
            "/region2/eq3/vel2freq,f : { 0 }",
            "/region2/eq1/frequency,f : { 500 }",
            "/region2/eq2/frequency,f : { 5000 }",
        };
        REQUIRE(messageList == expected);
    }
}

TEST_CASE("[Values] EQ types")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav eq1_type=hshelf
        <region> sample=kick.wav eq1_type=lshelf
        <region> sample=kick.wav eq1_type=hshelf eq1_type=peak
        <region> sample=kick.wav eq1_type=something
    )");

    synth.dispatchMessage(client, 0, "/region0/eq0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region1/eq0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region2/eq0/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region3/eq0/type", "", nullptr);
    std::vector<std::string> expected {
        "/region0/eq0/type,s : { hshelf }",
        "/region1/eq0/type,s : { lshelf }",
        "/region2/eq0/type,s : { peak }",
        "/region3/eq0/type,s : { none }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] EQ dispatching")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
        <region> sample=kick.wav
            eq3_bw=2 eq1_gain=-25 eq2_freq=300 eq3_type=lshelf
            eq3_vel2gain=10 eq1_vel2freq=100
    )");

    synth.dispatchMessage(client, 0, "/region0/eq2/bandwidth", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/eq0/gain", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/eq1/frequency", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/eq2/type", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/eq2/vel2gain", "", nullptr);
    synth.dispatchMessage(client, 0, "/region0/eq0/vel2freq", "", nullptr);
    std::vector<std::string> expected {
        "/region0/eq2/bandwidth,f : { 2 }",
        "/region0/eq0/gain,f : { -25 }",
        "/region0/eq1/frequency,f : { 300 }",
        "/region0/eq2/type,s : { lshelf }",
        "/region0/eq2/vel2gain,f : { 10 }",
        "/region0/eq0/vel2freq,f : { 100 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Values] EQ value bounds")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);

    SECTION("Frequency")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav eq1_freq=100000
            <region> sample=kick.wav eq1_freq=50 eq1_freq=-100
        )");
        synth.dispatchMessage(client, 0, "/region0/eq0/frequency", "", nullptr);
        synth.dispatchMessage(client, 0, "/region1/eq0/frequency", "", nullptr);
        std::vector<std::string> expected {
            "/region0/eq0/frequency,f : { 100000 }",
            "/region1/eq0/frequency,f : { -100 }",
        };
        REQUIRE(messageList == expected);
    }

    SECTION("Bandwidth")
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/value_tests.sfz", R"(
            <region> sample=kick.wav eq1_bw=5 eq1_bw=-5
        )");
        synth.dispatchMessage(client, 0, "/region0/eq0/bandwidth", "", nullptr);
        std::vector<std::string> expected {
            "/region0/eq0/bandwidth,f : { -5 }",
        };
        REQUIRE(messageList == expected);
    }
}
