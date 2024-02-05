// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Messaging.h"
#include "TestHelpers.h"
#include "sfizz/Synth.h"
#include "sfizz/Logger.h"
#include "catch2/catch.hpp"
#include <stdexcept>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>
using namespace Catch::literals;
using namespace sfz;

TEST_CASE("[Logging] Basic logging")
{
    Synth synth;
    Client client { nullptr };
    auto& logger = synth.getResources().getLogger();
    REQUIRE_FALSE(logger.subscribed("/log/default_path_changed"));
    logger.subscribe("/log/default_path_changed", client);
    REQUIRE(logger.subscribed("/log/default_path_changed"));
}

TEST_CASE("[Logging] Changing default path")
{
    Synth synth;
    std::vector<std::string> messageList;
    Client client { &messageList };
    client.setReceiveCallback(&simpleMessageReceiver);
    synth.subscribe("/log/default_path_changed", client);
    synth.loadSfzFile(fs::current_path() / "tests/TestFiles/default_path.sfz");
    std::vector<std::string> expected {
        "/log/default_path_changed,s : {  }",
        "/log/default_path_changed,s : { DefaultPath/SubPath2/ }",
        "/log/default_path_changed,s : {  }",
        "/log/default_path_changed,s : { DefaultPath/SubPath1/sample }",
        "/log/default_path_changed,s : {  }",
    };
    REQUIRE(messageList == expected);
}

