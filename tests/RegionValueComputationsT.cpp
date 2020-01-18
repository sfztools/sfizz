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

#include "sfizz/Defaults.h"
#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/MidiState.h"
#include "catch2/catch.hpp"
#include <chrono>
#include <thread>
using namespace Catch::literals;
constexpr int numRandomTests { 64 };
TEST_CASE("[Region] Crossfade in on key")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "3" });
    REQUIRE( region.getNoteGain(2, 127) == 0.70711_a );
    REQUIRE( region.getNoteGain(1, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(3, 127) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on key - 2")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    REQUIRE( region.getNoteGain(1, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(2, 127) == 0.5_a );
    REQUIRE( region.getNoteGain(3, 127) == 0.70711_a );
    REQUIRE( region.getNoteGain(4, 127) == 0.86603_a );
    REQUIRE( region.getNoteGain(5, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(6, 127) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on key - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE( region.getNoteGain(1, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(2, 127) == 0.25_a );
    REQUIRE( region.getNoteGain(3, 127) == 0.5_a );
    REQUIRE( region.getNoteGain(4, 127) == 0.75_a );
    REQUIRE( region.getNoteGain(5, 127) == 1.0_a );
}

TEST_CASE("[Region] Crossfade out on key")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    REQUIRE( region.getNoteGain(50, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(51, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(52, 127) == 0.86603_a );
    REQUIRE( region.getNoteGain(53, 127) == 0.70711_a );
    REQUIRE( region.getNoteGain(54, 127) == 0.5_a );
    REQUIRE( region.getNoteGain(55, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(56, 127) == 0.0_a );
}

TEST_CASE("[Region] Crossfade out on key - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE( region.getNoteGain(50, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(51, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(52, 127) == 0.75_a );
    REQUIRE( region.getNoteGain(53, 127) == 0.5_a );
    REQUIRE( region.getNoteGain(54, 127) == 0.25_a );
    REQUIRE( region.getNoteGain(55, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(56, 127) == 0.0_a );
}

TEST_CASE("[Region] Crossfade in on velocity")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE( region.getNoteGain(1, 19) == 0.0_a );
    REQUIRE( region.getNoteGain(1, 20) == 0.0_a );
    REQUIRE( region.getNoteGain(2, 21) == 0.5_a );
    REQUIRE( region.getNoteGain(3, 22) == 0.70711_a );
    REQUIRE( region.getNoteGain(4, 23) == 0.86603_a );
    REQUIRE( region.getNoteGain(5, 24) == 1.0_a );
    REQUIRE( region.getNoteGain(6, 25) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on vel - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE( region.getNoteGain(1, 19) == 0.0_a );
    REQUIRE( region.getNoteGain(1, 20) == 0.0_a );
    REQUIRE( region.getNoteGain(2, 21) == 0.25_a );
    REQUIRE( region.getNoteGain(3, 22) == 0.5_a );
    REQUIRE( region.getNoteGain(4, 23) == 0.75_a );
    REQUIRE( region.getNoteGain(5, 24) == 1.0_a );
    REQUIRE( region.getNoteGain(5, 25) == 1.0_a );
}

TEST_CASE("[Region] Crossfade out on vel")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE( region.getNoteGain(5, 50) == 1.0_a );
    REQUIRE( region.getNoteGain(5, 51) == 1.0_a );
    REQUIRE( region.getNoteGain(5, 52) == 0.86603_a );
    REQUIRE( region.getNoteGain(5, 53) == 0.70711_a );
    REQUIRE( region.getNoteGain(5, 54) == 0.5_a );
    REQUIRE( region.getNoteGain(5, 55) == 0.0_a );
    REQUIRE( region.getNoteGain(5, 56) == 0.0_a );
}

TEST_CASE("[Region] Crossfade out on vel - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE( region.getNoteGain(56, 50) == 1.0_a );
    REQUIRE( region.getNoteGain(56, 51) == 1.0_a );
    REQUIRE( region.getNoteGain(56, 52) == 0.75_a );
    REQUIRE( region.getNoteGain(56, 53) == 0.5_a );
    REQUIRE( region.getNoteGain(56, 54) == 0.25_a );
    REQUIRE( region.getNoteGain(56, 55) == 0.0_a );
    REQUIRE( region.getNoteGain(56, 56) == 0.0_a );
}

TEST_CASE("[Region] Crossfade in on CC")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
	midiState.ccEvent(24, 19); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 20); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 21); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.5_a );
	midiState.ccEvent(24, 22); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.70711_a );
	midiState.ccEvent(24, 23); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.86603_a );
	midiState.ccEvent(24, 24); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 25); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on CC - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
	midiState.ccEvent(24, 19); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 20); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 21); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.25_a );
	midiState.ccEvent(24, 22); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.5_a );
	midiState.ccEvent(24, 23); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.75_a );
	midiState.ccEvent(24, 24); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 25); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
}
TEST_CASE("[Region] Crossfade out on CC")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
	midiState.ccEvent(24, 19); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 20); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 21); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.86603_a );
	midiState.ccEvent(24, 22); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.70711_a );
	midiState.ccEvent(24, 23); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.5_a );
	midiState.ccEvent(24, 24); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 25); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
}

TEST_CASE("[Region] Crossfade out on CC - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
	midiState.ccEvent(24, 19); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 20); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 1.0_a );
	midiState.ccEvent(24, 21); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.75_a );
	midiState.ccEvent(24, 22); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.5_a );
	midiState.ccEvent(24, 23); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.25_a );
	midiState.ccEvent(24, 24); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
	midiState.ccEvent(24, 25); REQUIRE( region.getCrossfadeGain(midiState.getCCArray()) == 0.0_a );
}

TEST_CASE("[Region] Velocity bug for extreme values - veltrack at 0")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE( region.getNoteGain(64, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(64, 0) == 1.0_a );
}


TEST_CASE("[Region] Velocity bug for extreme values - positive veltrack")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "100" });
    REQUIRE( region.getNoteGain(64, 127) == 1.0_a );
    REQUIRE( region.getNoteGain(64, 0) == Approx(0.0).margin(0.0001) );
}

TEST_CASE("[Region] Velocity bug for extreme values - negative veltrack")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "-100" });
    REQUIRE( region.getNoteGain(64, 127) == Approx(0.0).margin(0.0001) );
    REQUIRE( region.getNoteGain(64, 0) == 1.0_a );
}

TEST_CASE("[Region] rt_decay")
{
    sfz::MidiState midiState;
    sfz::Region region { midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "trigger", "release" });
    region.parseOpcode({ "rt_decay", "10" });
    midiState.noteOnEvent(64, 64);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume - 1.0f).margin(0.1) );
    region.parseOpcode({ "rt_decay", "20" });
    midiState.noteOnEvent(64, 64);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume - 2.0f).margin(0.1) );
    region.parseOpcode({ "trigger", "attack" });
    midiState.noteOnEvent(64, 64);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume).margin(0.1) );
}

TEST_CASE("[Region] Base delay")
{
    sfz::MidiState midiState;
    sfz::Region region{ midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "delay", "10" });
    REQUIRE( region.getDelay() == 10.0f );
    region.parseOpcode({ "delay_random", "10" });
    Random::randomGenerator.seed(42);
    for (int i = 0; i < numRandomTests; ++i)
    {
        auto delay = region.getDelay();
        REQUIRE( (delay >= 10.0 && delay <= 20.0) );
    }
}
