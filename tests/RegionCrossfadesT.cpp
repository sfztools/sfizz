#include "../sources/Region.h"
#include "catch2/catch.hpp"
#include <SfzHelpers.h>
using namespace Catch::literals;

TEST_CASE("[Region] Crossfade in on key")
{
	sfz::Region region {};
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "3" });
    REQUIRE( region.getNoteGain(2, 127) == 0.70711_a );
    REQUIRE( region.getNoteGain(1, 127) == 0.0_a );
    REQUIRE( region.getNoteGain(3, 127) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on key - 2")
{
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
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
	sfz::Region region {};
	sfz::CCValueArray ccState;
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
	ccState[24] = 19; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 20; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 21; REQUIRE( region.getCCGain(ccState) == 0.5_a );
	ccState[24] = 22; REQUIRE( region.getCCGain(ccState) == 0.70711_a );
	ccState[24] = 23; REQUIRE( region.getCCGain(ccState) == 0.86603_a );
	ccState[24] = 24; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 25; REQUIRE( region.getCCGain(ccState) == 1.0_a );
}

TEST_CASE("[Region] Crossfade in on CC - gain")
{
	sfz::Region region {};
	sfz::CCValueArray ccState;
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
	ccState[24] = 19; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 20; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 21; REQUIRE( region.getCCGain(ccState) == 0.25_a );
	ccState[24] = 22; REQUIRE( region.getCCGain(ccState) == 0.5_a );
	ccState[24] = 23; REQUIRE( region.getCCGain(ccState) == 0.75_a );
	ccState[24] = 24; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 25; REQUIRE( region.getCCGain(ccState) == 1.0_a );
}
TEST_CASE("[Region] Crossfade out on CC")
{
	sfz::Region region {};
	sfz::CCValueArray ccState;
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
	ccState[24] = 19; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 20; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 21; REQUIRE( region.getCCGain(ccState) == 0.86603_a );
	ccState[24] = 22; REQUIRE( region.getCCGain(ccState) == 0.70711_a );
	ccState[24] = 23; REQUIRE( region.getCCGain(ccState) == 0.5_a );
	ccState[24] = 24; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 25; REQUIRE( region.getCCGain(ccState) == 0.0_a );
}

TEST_CASE("[Region] Crossfade out on CC - gain")
{
	sfz::Region region {};
	sfz::CCValueArray ccState;
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
	ccState[24] = 19; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 20; REQUIRE( region.getCCGain(ccState) == 1.0_a );
	ccState[24] = 21; REQUIRE( region.getCCGain(ccState) == 0.75_a );
	ccState[24] = 22; REQUIRE( region.getCCGain(ccState) == 0.5_a );
	ccState[24] = 23; REQUIRE( region.getCCGain(ccState) == 0.25_a );
	ccState[24] = 24; REQUIRE( region.getCCGain(ccState) == 0.0_a );
	ccState[24] = 25; REQUIRE( region.getCCGain(ccState) == 0.0_a );
}