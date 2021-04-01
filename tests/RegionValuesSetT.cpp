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

TEST_CASE("[Set values] Pitch keycenter")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    AudioBuffer<float> buffer { 2, 256 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/values_set.sfz", R"(
        <region> sample=*sine pitch_keycenter=48
    )");
    synth.dispatchMessage(client, 0, "/region0/pitch_keycenter", "", nullptr);

    // Update value
    sfizz_arg_t args;
    args.i = 60;
    synth.dispatchMessage(client, 1, "/region0/pitch_keycenter", "i", &args);
    synth.renderBlock(buffer);

    synth.dispatchMessage(client, 0, "/region0/pitch_keycenter", "", nullptr);
    std::vector<std::string> expected {
        "/region0/pitch_keycenter,i : { 48 }",
        "/region0/pitch_keycenter,i : { 60 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Set values] LFO wave")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    AudioBuffer<float> buffer { 2, 256 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/values_set.sfz", R"(
        <region> sample=*sine lfo1_wave=5
    )");
    synth.dispatchMessage(client, 0, "/region0/lfo0/wave", "", nullptr);

    // Update value
    sfizz_arg_t args;
    args.i = 2;
    synth.dispatchMessage(client, 1, "/region0/lfo0/wave", "i", &args);
    synth.renderBlock(buffer);

    synth.dispatchMessage(client, 0, "/region0/lfo0/wave", "", nullptr);
    std::vector<std::string> expected {
        "/region0/lfo0/wave,i : { 5 }",
        "/region0/lfo0/wave,i : { 2 }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Set values] Filter type")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    AudioBuffer<float> buffer { 2, 256 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/values_set.sfz", R"(
        <region> sample=*sine fil2_type=lpf_1p
    )");
    synth.dispatchMessage(client, 0, "/region0/filter1/type", "", nullptr);

    // Update value
    sfizz_arg_t args;
    args.s = "hpf_2p";
    synth.dispatchMessage(client, 1, "/region0/filter1/type", "s", &args);
    synth.renderBlock(buffer);

    synth.dispatchMessage(client, 0, "/region0/filter1/type", "", nullptr);
    std::vector<std::string> expected {
        "/region0/filter1/type,s : { lpf_1p }",
        "/region0/filter1/type,s : { hpf_2p }",
    };
    REQUIRE(messageList == expected);
}

TEST_CASE("[Set values] Loop mode")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client(&messageList);
    client.setReceiveCallback(&simpleMessageReceiver);
    AudioBuffer<float> buffer { 2, 256 };

    synth.loadSfzString(fs::current_path() / "tests/TestFiles/values_set.sfz", R"(
        <region> sample=looped_flute.wav
    )");
    synth.dispatchMessage(client, 0, "/region0/loop_mode", "", nullptr);

    // Update value
    sfizz_arg_t args;
    args.s = "one_shot";
    synth.dispatchMessage(client, 1, "/region0/loop_mode", "s", &args);
    synth.renderBlock(buffer);

    synth.dispatchMessage(client, 0, "/region0/loop_mode", "", nullptr);
    std::vector<std::string> expected {
        "/region0/loop_mode,s : { loop_continuous }",
        "/region0/loop_mode,s : { one_shot }",
    };
    REQUIRE(messageList == expected);
}
