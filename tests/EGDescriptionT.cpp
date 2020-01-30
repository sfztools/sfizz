// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz


#include "sfizz/EGDescription.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[EGDescription] Attack range")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.attack = 1;
    eg.vel2attack = -1.27f;
    eg.ccAttack = { 63, 1.27f };
    REQUIRE( eg.getAttack(ccArray, 0) == 1.0f );
    REQUIRE( eg.getAttack(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getAttack(ccArray, 127) == 1.0f );
    REQUIRE( eg.getAttack(ccArray, 0) == 2.27f );
    eg.ccAttack = { 63, 127.0f };
    REQUIRE( eg.getAttack(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Delay range")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.delay = 1;
    eg.vel2delay = -1.27f;
    eg.ccDelay = { 63, 1.27f };
    REQUIRE( eg.getDelay(ccArray, 0) == 1.0f );
    REQUIRE( eg.getDelay(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getDelay(ccArray, 127) == 1.0f );
    REQUIRE( eg.getDelay(ccArray, 0) == 2.27f );
    eg.ccDelay = { 63, 127.0f };
    REQUIRE( eg.getDelay(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Decay range")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.decay = 1.0f;
    eg.vel2decay = -1.27f;
    eg.ccDecay = { 63, 1.27f };
    REQUIRE( eg.getDecay(ccArray, 0) == 1.0f );
    REQUIRE( eg.getDecay(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getDecay(ccArray, 127) == 1.0f );
    REQUIRE( eg.getDecay(ccArray, 0) == 2.27f );
    eg.ccDecay = { 63, 127.0f };
    REQUIRE( eg.getDecay(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Release range")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.release = 1;
    eg.vel2release = -1.27f;
    eg.ccRelease = { 63, 1.27f };
    REQUIRE( eg.getRelease(ccArray, 0) == 1.0f );
    REQUIRE( eg.getRelease(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getRelease(ccArray, 127) == 1.0f );
    REQUIRE( eg.getRelease(ccArray, 0) == 2.27f );
    eg.ccRelease = { 63, 127.0f };
    REQUIRE( eg.getRelease(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Hold range")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.hold = 1;
    eg.vel2hold = -1.27f;
    eg.ccHold = { 63, 1.27f };
    REQUIRE( eg.getHold(ccArray, 0) == 1.0f );
    REQUIRE( eg.getHold(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getHold(ccArray, 127) == 1.0f );
    REQUIRE( eg.getHold(ccArray, 0) == 2.27f );
    eg.ccHold = { 63, 127.0f };
    REQUIRE( eg.getHold(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Sustain level")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.sustain = 50;
    eg.vel2sustain = -100;
    eg.ccSustain = { 63, 100.0f };
    REQUIRE( eg.getSustain(ccArray, 0) == 50.0f );
    REQUIRE( eg.getSustain(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getSustain(ccArray, 127) == 50.0f );
    eg.ccSustain = { 63, 200.0f };
    REQUIRE( eg.getSustain(ccArray, 0) == 100.0f );
}

TEST_CASE("[EGDescription] Start level")
{
    sfz::EGDescription eg;
    sfz::SfzCCArray ccArray { 0 };
    eg.start = 0;
    eg.ccStart = { 63, 127.0f };
    REQUIRE( eg.getStart(ccArray, 0) == 0.0f );
    REQUIRE( eg.getStart(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getStart(ccArray, 0) == 100.0f );
    eg.ccStart = { 63, -127.0f };
    REQUIRE( eg.getStart(ccArray, 0) == 0.0f );
}
