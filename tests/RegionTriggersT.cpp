#include "../sources/Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("Basic triggers", "Region triggers")
{
    sfz::Region region;
    region.parseOpcode({ "sample", "*sine" });
    SECTION("key")
    {
        region.parseOpcode({ "key", "40" });
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(1, 63, 64));
    }
    SECTION("lokey and hikey")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "42" });
        REQUIRE(!region.registerNoteOn(1, 39, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 41, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 42, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 43, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 42, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 42, 64, 0.5f));
        REQUIRE(!region.registerCC(1, 63, 64));
    }
    SECTION("key and release trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOff(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(1, 63, 64));
    }
    SECTION("key and release_key trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release_key" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOff(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 64, 0.5f));
        REQUIRE(!region.registerNoteOff(1, 41, 64, 0.5f));
        REQUIRE(!region.registerCC(1, 63, 64));
    }
    // TODO: first and legato triggers
    SECTION("lovel and hivel")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lovel", "60" });
        region.parseOpcode({ "hivel", "70" });
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 40, 60, 0.5f));
        REQUIRE(region.registerNoteOn(1, 40, 70, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 71, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 59, 0.5f));
    }
    SECTION("lochan and hichan")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lochan", "2" });
        region.parseOpcode({ "hichan", "4" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(2, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(3, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(4, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(5, 40, 64, 0.5f));
    }

    SECTION("lorand and hirand")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.34f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.35f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.36f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.37f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.38f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.39f));
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.40f));
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.41f));
    }

    SECTION("lorand and hirand on 1.0f")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.34f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.35f));
        REQUIRE(region.registerNoteOn(1, 40, 64, 1.0f));
    }

    SECTION("on_loccN, on_hiccN")
    {
        region.parseOpcode({ "on_locc47", "64" });
        region.parseOpcode({ "on_hicc47", "68" });
        REQUIRE(!region.registerCC(1, 47, 63));
        REQUIRE(region.registerCC(1, 47, 64));
        REQUIRE(region.registerCC(1, 47, 65));
        REQUIRE(region.registerCC(1, 47, 66));
        REQUIRE(region.registerCC(1, 47, 67));
        REQUIRE(region.registerCC(1, 47, 68));
        REQUIRE(!region.registerCC(1, 47, 69));
        REQUIRE(!region.registerCC(1, 40, 64));
    }
}

TEST_CASE("Legato triggers", "Region triggers")
{
    sfz::Region region;
    region.parseOpcode({ "sample", "*sine" });
    SECTION("First note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "first" });
        REQUIRE(region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(!region.registerNoteOn(1, 41, 64, 0.5f));
        region.registerNoteOff(1, 40, 0, 0.5f);
        region.registerNoteOff(1, 41, 0, 0.5f);
        REQUIRE(region.registerNoteOn(1, 42, 64, 0.5f));
    }

    SECTION("Second note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "legato" });
        REQUIRE(!region.registerNoteOn(1, 40, 64, 0.5f));
        REQUIRE(region.registerNoteOn(1, 41, 64, 0.5f));
        region.registerNoteOff(1, 40, 0, 0.5f);
        region.registerNoteOff(1, 41, 0, 0.5f);
        REQUIRE(!region.registerNoteOn(1, 42, 64, 0.5f));
    }
}