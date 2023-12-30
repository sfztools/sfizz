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

TEST_CASE("[Logging] Basic logging")
{
    Synth synth;
    std::vector<std::string> messageList;
    synth.setBroadcastCallback(&simpleMessageReceiver, &messageList);

    Client client { &messageList };
    synth.dispatchMessage(client, 0, "/log_level", "", nullptr);
    sfizz_arg_t args;
    args.s = "TRACE";
    synth.dispatchMessage(client, 0, "/log_level", "s", &args);
    synth.dispatchMessage(client, 0, "/log_level", "", nullptr);
    args.s = "INFO";
    synth.dispatchMessage(client, 0, "/log_level", "s", &args);
    synth.dispatchMessage(client, 0, "/log_level", "", nullptr);
    args.s = "WARNING";
    synth.dispatchMessage(client, 0, "/log_level", "s", &args);
    synth.dispatchMessage(client, 0, "/log_level", "", nullptr);

    std::vector<std::string> expected {
        "/log_level,s : { ERROR }",
        "/log_level,s : { TRACE }",
        "/log_level,s : { INFO }",
        "/log_level,s : { WARNING }",
    };
    REQUIRE(messageList == expected);
}
