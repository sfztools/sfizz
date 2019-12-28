// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "EGDescription.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[EGDescription] Attack range")
{
    sfz::EGDescription eg;
    eg.attack = 1;
    eg.vel2attack = -1.27;
    eg.ccAttack = { 63, 1.27f };
    sfz::SfzCCArray ccArray;
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
    eg.delay = 1;
    eg.vel2delay = -1.27;
    eg.ccDelay = { 63, 1.27f };
    sfz::SfzCCArray ccArray;
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
    eg.decay = 1;
    eg.vel2decay = -1.27;
    eg.ccDecay = { 63, 1.27f };
    sfz::SfzCCArray ccArray;
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
    eg.release = 1;
    eg.vel2release = -1.27;
    eg.ccRelease = { 63, 1.27f };
    sfz::SfzCCArray ccArray;
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
    eg.hold = 1;
    eg.vel2hold = -1.27;
    eg.ccHold = { 63, 1.27f };
    sfz::SfzCCArray ccArray;
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
    eg.sustain = 50;
    eg.vel2sustain = -100;
    eg.ccSustain = { 63, 100.0f };
    sfz::SfzCCArray ccArray;
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
    eg.start = 0;
    eg.ccStart = { 63, 127.0f };
    sfz::SfzCCArray ccArray;
    REQUIRE( eg.getStart(ccArray, 0) == 0.0f );
    REQUIRE( eg.getStart(ccArray, 127) == 0.0f );
    ccArray[63] = 127;
    REQUIRE( eg.getStart(ccArray, 0) == 100.0f );
    eg.ccStart = { 63, -127.0f };
    REQUIRE( eg.getStart(ccArray, 0) == 0.0f );
}