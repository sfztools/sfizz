// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "RegionTHelpers.h"
#include "sfizz/MidiState.h"
#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/modulations/ModId.h"
#include "sfizz/modulations/ModKey.h"
#include "catch2/catch.hpp"
#include <stdexcept>
using namespace Catch::literals;
using namespace sfz::literals;
using namespace sfz;

TEST_CASE("[Region] Parsing opcodes")
{
    MidiState midiState;
    Region region { 0, midiState };

    SECTION("sample")
    {
        REQUIRE(region.sampleId.filename() == "");
        region.parseOpcode({ "sample", "dummy.wav" });
        REQUIRE(region.sampleId.filename() == "dummy.wav");
    }

    SECTION("direction")
    {
        REQUIRE(!region.sampleId.isReverse());
        region.parseOpcode({ "direction", "reverse" });
        REQUIRE(region.sampleId.isReverse());
        region.parseOpcode({ "direction", "forward" });
        REQUIRE(!region.sampleId.isReverse());
    }

    SECTION("delay")
    {
        REQUIRE(region.delay == 0.0);
        region.parseOpcode({ "delay", "1.0" });
        REQUIRE(region.delay == 1.0);
        region.parseOpcode({ "delay", "-1.0" });
        REQUIRE(region.delay == 0.0);
        region.parseOpcode({ "delay", "110.0" });
        REQUIRE(region.delay == 100.0);
    }

    SECTION("delay_random")
    {
        REQUIRE(region.delayRandom == 0.0);
        region.parseOpcode({ "delay_random", "1.0" });
        REQUIRE(region.delayRandom == 1.0);
        region.parseOpcode({ "delay_random", "-1.0" });
        REQUIRE(region.delayRandom == 0.0);
        region.parseOpcode({ "delay_random", "110.0" });
        REQUIRE(region.delayRandom == 100.0);
    }

    SECTION("offset")
    {
        REQUIRE(region.offset == 0);
        region.parseOpcode({ "offset", "1" });
        REQUIRE(region.offset == 1);
        region.parseOpcode({ "offset", "-1" });
        REQUIRE(region.offset == 0);
    }
    SECTION("offset_cc")
    {
        REQUIRE(region.offsetCC.empty());
        region.parseOpcode({ "offset_cc1", "1" });
        REQUIRE(region.offsetCC.contains(1));
        REQUIRE(region.offsetCC[1] == 1);
        region.parseOpcode({ "offset_cc2", "15420" });
        REQUIRE(region.offsetCC.contains(2));
        REQUIRE(region.offsetCC[2] == 15420);
        region.parseOpcode({ "offset_cc2", "-1" });
        REQUIRE(region.offsetCC[2] == 0);
    }


    SECTION("offset_random")
    {
        REQUIRE(region.offsetRandom == 0);
        region.parseOpcode({ "offset_random", "1" });
        REQUIRE(region.offsetRandom == 1);
        region.parseOpcode({ "offset_random", "-1" });
        REQUIRE(region.offsetRandom == 0);
    }

    SECTION("end")
    {
        region.parseOpcode({ "end", "184" });
        REQUIRE(region.sampleEnd == 184);
        region.parseOpcode({ "end", "-1" });
        REQUIRE(region.sampleEnd == 0);
    }

    SECTION("count")
    {
        REQUIRE(!region.sampleCount);
        region.parseOpcode({ "count", "184" });
        REQUIRE(region.sampleCount);
        REQUIRE(*region.sampleCount == 184);
        region.parseOpcode({ "count", "-1" });
        REQUIRE(region.sampleCount);
        REQUIRE(*region.sampleCount == 0);
    }

    SECTION("loop_mode")
    {
        REQUIRE( !region.loopMode );
        region.parseOpcode({ "loop_mode", "no_loop" });
        REQUIRE(region.loopMode == SfzLoopMode::no_loop);
        region.parseOpcode({ "loop_mode", "one_shot" });
        REQUIRE(region.loopMode == SfzLoopMode::one_shot);
        region.parseOpcode({ "loop_mode", "loop_continuous" });
        REQUIRE(region.loopMode == SfzLoopMode::loop_continuous);
        region.parseOpcode({ "loop_mode", "loop_sustain" });
        REQUIRE(region.loopMode == SfzLoopMode::loop_sustain);
    }

    SECTION("loopmode")
    {
        REQUIRE( !region.loopMode );
        region.parseOpcode({ "loopmode", "no_loop" });
        REQUIRE(region.loopMode == SfzLoopMode::no_loop);
        region.parseOpcode({ "loopmode", "one_shot" });
        REQUIRE(region.loopMode == SfzLoopMode::one_shot);
        region.parseOpcode({ "loopmode", "loop_continuous" });
        REQUIRE(region.loopMode == SfzLoopMode::loop_continuous);
        region.parseOpcode({ "loopmode", "loop_sustain" });
        REQUIRE(region.loopMode == SfzLoopMode::loop_sustain);
    }

    SECTION("loop_end")
    {
        REQUIRE(region.loopRange == Range<uint32_t>(0, 4294967295));
        region.parseOpcode({ "loop_end", "184" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 184));
        region.parseOpcode({ "loop_end", "-1" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 0));
    }

    SECTION("loop_start")
    {
        region.parseOpcode({ "loop_start", "184" });
        REQUIRE(region.loopRange == Range<uint32_t>(184, 4294967295));
        region.parseOpcode({ "loop_start", "-1" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 4294967295));
    }

    SECTION("loopend")
    {
        REQUIRE(region.loopRange == Range<uint32_t>(0, 4294967295));
        region.parseOpcode({ "loopend", "184" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 184));
        region.parseOpcode({ "loopend", "-1" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 0));
    }

    SECTION("loopstart")
    {
        region.parseOpcode({ "loopstart", "184" });
        REQUIRE(region.loopRange == Range<uint32_t>(184, 4294967295));
        region.parseOpcode({ "loopstart", "-1" });
        REQUIRE(region.loopRange == Range<uint32_t>(0, 4294967295));
    }

    SECTION("group")
    {
        REQUIRE(region.group == 0);
        region.parseOpcode({ "group", "5" });
        REQUIRE(region.group == 5);
        region.parseOpcode({ "group", "-1" });
        REQUIRE(region.group == 0);
    }

    SECTION("off_by")
    {
        REQUIRE(!region.offBy);
        region.parseOpcode({ "off_by", "5" });
        REQUIRE(region.offBy);
        REQUIRE(*region.offBy == 5);
        region.parseOpcode({ "off_by", "-1" });
        REQUIRE(!region.offBy);
    }

    SECTION("off_mode")
    {
        REQUIRE(region.offMode == SfzOffMode::fast);
        region.parseOpcode({ "off_mode", "fast" });
        REQUIRE(region.offMode == SfzOffMode::fast);
        region.parseOpcode({ "off_mode", "normal" });
        REQUIRE(region.offMode == SfzOffMode::normal);
    }

    SECTION("lokey, hikey, and key")
    {
        REQUIRE(region.keyRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "lokey", "37" });
        REQUIRE(region.keyRange == Range<uint8_t>(37, 127));
        region.parseOpcode({ "lokey", "c4" });
        REQUIRE(region.keyRange == Range<uint8_t>(60, 127));
        region.parseOpcode({ "lokey", "128" });
        REQUIRE(region.keyRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "lokey", "-3" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "hikey", "65" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 65));
        region.parseOpcode({ "hikey", "c4" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 60));
        region.parseOpcode({ "hikey", "-1" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 0));
        region.parseOpcode({ "hikey", "128" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "key", "26" });
        REQUIRE(region.keyRange == Range<uint8_t>(26, 26));
        REQUIRE(region.pitchKeycenter == 26);
        region.parseOpcode({ "key", "-26" });
        REQUIRE(region.keyRange == Range<uint8_t>(0, 0));
        REQUIRE(region.pitchKeycenter == 0);
        region.parseOpcode({ "key", "234" });
        REQUIRE(region.keyRange == Range<uint8_t>(127, 127));
        REQUIRE(region.pitchKeycenter == 127);
        region.parseOpcode({ "key", "c4" });
        REQUIRE(region.keyRange == Range<uint8_t>(60, 60));
        REQUIRE(region.pitchKeycenter == 60);
    }

    SECTION("lovel, hivel")
    {
        REQUIRE(region.velocityRange == Range<float>(0_norm, 127_norm));
        region.parseOpcode({ "lovel", "37" });
        REQUIRE(region.velocityRange == Range<float>(37_norm, 127_norm));
        region.parseOpcode({ "lovel", "128" });
        REQUIRE(region.velocityRange == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "lovel", "-3" });
        REQUIRE(region.velocityRange == Range<float>(0_norm, 127_norm));
        region.parseOpcode({ "hivel", "65" });
        REQUIRE(region.velocityRange == Range<float>(0_norm, 65_norm));
        region.parseOpcode({ "hivel", "-1" });
        REQUIRE(region.velocityRange == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "hivel", "128" });
        REQUIRE(region.velocityRange == Range<float>(0_norm, 127_norm));
    }

    SECTION("lobend, hibend")
    {
        REQUIRE(region.bendRange == Range<float>(-1.0f, 1.0f));
        region.parseOpcode({ "lobend", "400" });
        REQUIRE(region.bendRange.getStart() == Approx(normalizeBend(400)));
        REQUIRE(region.bendRange.getEnd() == 1.0_a);
        region.parseOpcode({ "lobend", "-128" });
        REQUIRE(region.bendRange.getStart() == Approx(normalizeBend(-128)));
        REQUIRE(region.bendRange.getEnd() == 1.0_a);
        region.parseOpcode({ "lobend", "-10000" });
        REQUIRE(region.bendRange == Range<float>(-1.0f, 1.0f));
        region.parseOpcode({ "hibend", "13" });
        REQUIRE(region.bendRange.getStart() == -1.0_a);
        REQUIRE(region.bendRange.getEnd() == Approx(normalizeBend(13)));
        region.parseOpcode({ "hibend", "-1" });
        REQUIRE(region.bendRange.getStart() == -1.0_a);
        REQUIRE(region.bendRange.getEnd() == Approx(normalizeBend(-1)));
        region.parseOpcode({ "hibend", "10000" });
        REQUIRE(region.bendRange == Range<float>(-1.0f, 1.0f));
    }

    SECTION("locc, hicc")
    {
        REQUIRE(region.ccConditions.getWithDefault(0) == Range<float>(0_norm, 127_norm));
        REQUIRE(region.ccConditions[127] == Range<float>(0_norm, 127_norm));
        region.parseOpcode({ "locc6", "4" });
        REQUIRE(region.ccConditions[6] == Range<float>(4_norm, 127_norm));
        region.parseOpcode({ "locc12", "-128" });
        REQUIRE(region.ccConditions[12] == Range<float>(0_norm, 127_norm));
        region.parseOpcode({ "hicc65", "39" });
        REQUIRE(region.ccConditions[65] == Range<float>(0_norm, 39_norm));
        region.parseOpcode({ "hicc127", "135" });
        REQUIRE(region.ccConditions[127] == Range<float>(0_norm, 127_norm));
    }

    SECTION("lohdcc, hihdcc")
    {
        region.parseOpcode({ "lohdcc7", "0.12" });
        REQUIRE(region.ccConditions[7].getStart() == Approx(0.12f));
        REQUIRE(region.ccConditions[7].getEnd() == 1.0f);
        region.parseOpcode({ "lohdcc13", "-1.0" });
        REQUIRE(region.ccConditions[13] == sfz::Range<float>(0.0f, 1.0f));
        region.parseOpcode({ "hihdcc64", "0.45" });
        REQUIRE(region.ccConditions[64].getStart() == 0.0f);
        REQUIRE(region.ccConditions[64].getEnd() == Approx(0.45f));
        region.parseOpcode({ "hihdcc126", "1.2" });
        REQUIRE(region.ccConditions[126] == sfz::Range<float>(0.0f, 1.0f));
    }

    SECTION("lorealcc, hirealcc")
    {
        region.parseOpcode({ "lorealcc8", "0.12" });
        REQUIRE(region.ccConditions[8].getStart() == Approx(0.12f));
        REQUIRE(region.ccConditions[8].getEnd() == 1.0f);
        region.parseOpcode({ "lorealcc14", "-1.0" });
        REQUIRE(region.ccConditions[14] == sfz::Range<float>(0.0f, 1.0f));
        region.parseOpcode({ "hirealcc63", "0.45" });
        REQUIRE(region.ccConditions[63].getStart() == 0.0f);
        REQUIRE(region.ccConditions[63].getEnd() == Approx(0.45f));
        region.parseOpcode({ "hirealcc125", "1.2" });
        REQUIRE(region.ccConditions[125] == sfz::Range<float>(0.0f, 1.0f));
    }

    SECTION("sw_lokey, sw_hikey")
    {
        REQUIRE(region.keyswitchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "sw_lokey", "4" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(4, 127));
        region.parseOpcode({ "sw_lokey", "128" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "sw_lokey", "0" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "sw_hikey", "39" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(0, 39));
        region.parseOpcode({ "sw_hikey", "135" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "sw_hikey", "-1" });
        REQUIRE(region.keyswitchRange == Range<uint8_t>(0, 0));
    }

    SECTION("sw_label")
    {
        REQUIRE(!region.keyswitchLabel);
        region.parseOpcode({ "sw_label", "note" });
        REQUIRE(region.keyswitchLabel == "note");
        region.parseOpcode({ "sw_label", "ring" });
        REQUIRE(region.keyswitchLabel == "ring");
    }

    SECTION("sw_last")
    {
        REQUIRE(!region.keyswitch);
        region.parseOpcode({ "sw_last", "4" });
        REQUIRE(region.keyswitch);
        REQUIRE(*region.keyswitch == 4);
        region.parseOpcode({ "sw_last", "128" });
        REQUIRE(region.keyswitch);
        REQUIRE(*region.keyswitch == 127);
        region.parseOpcode({ "sw_last", "-1" });
        REQUIRE(region.keyswitch);
        REQUIRE(*region.keyswitch == 0);
    }

    SECTION("sw_up")
    {
        REQUIRE(!region.keyswitchUp);
        region.parseOpcode({ "sw_up", "4" });
        REQUIRE(region.keyswitchUp);
        REQUIRE(*region.keyswitchUp == 4);
        region.parseOpcode({ "sw_up", "128" });
        REQUIRE(region.keyswitchUp);
        REQUIRE(*region.keyswitchUp == 127);
        region.parseOpcode({ "sw_up", "-1" });
        REQUIRE(region.keyswitchUp);
        REQUIRE(*region.keyswitchUp == 0);
    }

    SECTION("sw_down")
    {
        REQUIRE(!region.keyswitchDown);
        region.parseOpcode({ "sw_down", "4" });
        REQUIRE(region.keyswitchDown);
        REQUIRE(*region.keyswitchDown == 4);
        region.parseOpcode({ "sw_down", "128" });
        REQUIRE(region.keyswitchDown);
        REQUIRE(*region.keyswitchDown == 127);
        region.parseOpcode({ "sw_down", "-1" });
        REQUIRE(region.keyswitchDown);
        REQUIRE(*region.keyswitchDown == 0);
    }

    SECTION("sw_previous")
    {
        REQUIRE(!region.previousNote);
        region.parseOpcode({ "sw_previous", "4" });
        REQUIRE(region.previousNote);
        REQUIRE(*region.previousNote == 4);
        region.parseOpcode({ "sw_previous", "128" });
        REQUIRE(region.previousNote);
        REQUIRE(*region.previousNote == 127);
        region.parseOpcode({ "sw_previous", "-1" });
        REQUIRE(region.previousNote);
        REQUIRE(*region.previousNote == 0);
    }

    SECTION("sw_vel")
    {
        REQUIRE(region.velocityOverride == SfzVelocityOverride::current);
        region.parseOpcode({ "sw_vel", "current" });
        REQUIRE(region.velocityOverride == SfzVelocityOverride::current);
        region.parseOpcode({ "sw_vel", "previous" });
        REQUIRE(region.velocityOverride == SfzVelocityOverride::previous);
    }

    SECTION("lochanaft, hichanaft")
    {
        REQUIRE(region.aftertouchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "lochanaft", "4" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(4, 127));
        region.parseOpcode({ "lochanaft", "128" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "lochanaft", "0" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "hichanaft", "39" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(0, 39));
        region.parseOpcode({ "hichanaft", "135" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(0, 127));
        region.parseOpcode({ "hichanaft", "-1" });
        REQUIRE(region.aftertouchRange == Range<uint8_t>(0, 0));
    }

    SECTION("lobpm, hibpm")
    {
        REQUIRE(region.bpmRange == Range<float>(0, 500));
        region.parseOpcode({ "lobpm", "47.5" });
        REQUIRE(region.bpmRange == Range<float>(47.5, 500));
        region.parseOpcode({ "lobpm", "594" });
        REQUIRE(region.bpmRange == Range<float>(500, 500));
        region.parseOpcode({ "lobpm", "0" });
        REQUIRE(region.bpmRange == Range<float>(0, 500));
        region.parseOpcode({ "hibpm", "78" });
        REQUIRE(region.bpmRange == Range<float>(0, 78));
        region.parseOpcode({ "hibpm", "895.4" });
        REQUIRE(region.bpmRange == Range<float>(0, 500));
        region.parseOpcode({ "hibpm", "-1" });
        REQUIRE(region.bpmRange == Range<float>(0, 0));
    }

    SECTION("lorand, hirand")
    {
        REQUIRE(region.randRange == Range<float>(0, 1));
        region.parseOpcode({ "lorand", "0.5" });
        REQUIRE(region.randRange == Range<float>(0.5, 1));
        region.parseOpcode({ "lorand", "4" });
        REQUIRE(region.randRange == Range<float>(1, 1));
        region.parseOpcode({ "lorand", "0" });
        REQUIRE(region.randRange == Range<float>(0, 1));
        region.parseOpcode({ "hirand", "39" });
        REQUIRE(region.randRange == Range<float>(0, 1));
        region.parseOpcode({ "hirand", "0.7" });
        REQUIRE(region.randRange == Range<float>(0, 0.7f));
        region.parseOpcode({ "hirand", "-1" });
        REQUIRE(region.randRange == Range<float>(0, 0));
    }

    SECTION("seq_length")
    {
        REQUIRE(region.sequenceLength == 1);
        region.parseOpcode({ "seq_length", "89" });
        REQUIRE(region.sequenceLength == 89);
        region.parseOpcode({ "seq_length", "189" });
        REQUIRE(region.sequenceLength == 100);
        region.parseOpcode({ "seq_length", "-1" });
        REQUIRE(region.sequenceLength == 1);
    }

    SECTION("seq_position")
    {
        REQUIRE(region.sequencePosition == 1);
        region.parseOpcode({ "seq_position", "89" });
        REQUIRE(region.sequencePosition == 89);
        region.parseOpcode({ "seq_position", "189" });
        REQUIRE(region.sequencePosition == 100);
        region.parseOpcode({ "seq_position", "-1" });
        REQUIRE(region.sequencePosition == 1);
    }

    SECTION("trigger")
    {
        REQUIRE(region.trigger == SfzTrigger::attack);
        region.parseOpcode({ "trigger", "attack" });
        REQUIRE(region.trigger == SfzTrigger::attack);
        region.parseOpcode({ "trigger", "release" });
        REQUIRE(region.trigger == SfzTrigger::release);
        region.parseOpcode({ "trigger", "first" });
        REQUIRE(region.trigger == SfzTrigger::first);
        region.parseOpcode({ "trigger", "legato" });
        REQUIRE(region.trigger == SfzTrigger::legato);
    }

    SECTION("on_locc, on_hicc")
    {
        for (int ccIdx = 1; ccIdx < 128; ++ccIdx) {
            REQUIRE(!region.ccTriggers.contains(ccIdx));
        }
        region.parseOpcode({ "on_locc45", "15" });
        REQUIRE(region.ccTriggers.contains(45));
        REQUIRE(region.ccTriggers[45] == Range<float>(15_norm, 127_norm));
        region.parseOpcode({ "on_hicc4", "47" });
        REQUIRE(region.ccTriggers.contains(45));
        REQUIRE(region.ccTriggers[4] == Range<float>(0_norm, 47_norm));
    }

    SECTION("on_lohdcc, on_hihdcc")
    {
        for (int ccIdx = 1; ccIdx < 128; ++ccIdx) {
            REQUIRE(!region.ccTriggers.contains(ccIdx));
        }
        region.parseOpcode({ "on_lohdcc46", "0.15" });
        REQUIRE(region.ccTriggers.contains(46));
        REQUIRE(region.ccTriggers[46].getStart() == Approx(0.15f));
        REQUIRE(region.ccTriggers[46].getEnd() == 1.0f);
        region.parseOpcode({ "on_hihdcc5", "0.47" });
        REQUIRE(region.ccTriggers.contains(5));
        REQUIRE(region.ccTriggers[5].getStart() == 0.0f);
        REQUIRE(region.ccTriggers[5].getEnd() == Approx(0.47f));
    }

    SECTION("volume")
    {
        REQUIRE(region.volume == 0.0f);
        region.parseOpcode({ "volume", "4.2" });
        REQUIRE(region.volume == 4.2f);
        region.parseOpcode({ "volume", "-4.2" });
        REQUIRE(region.volume == -4.2f);
        region.parseOpcode({ "volume", "-123" });
        REQUIRE(region.volume == -123.0f);
        region.parseOpcode({ "volume", "-185" });
        REQUIRE(region.volume == -144.0f);
        region.parseOpcode({ "volume", "79" });
        REQUIRE(region.volume == 48.0f);
    }

    SECTION("pan")
    {
        REQUIRE(region.pan == 0.0f);
        region.parseOpcode({ "pan", "4.2" });
        REQUIRE(region.pan == 0.042_a);
        region.parseOpcode({ "pan", "-4.2" });
        REQUIRE(region.pan == -0.042_a);
        region.parseOpcode({ "pan", "-123" });
        REQUIRE(region.pan == -1.0_a);
        region.parseOpcode({ "pan", "132" });
        REQUIRE(region.pan == 1.0_a);
    }

    SECTION("pan_oncc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Pan, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "pan_oncc45", "4.2" });
        REQUIRE(view.at(45).value == 4.2_a);
        region.parseOpcode({ "pan_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "pan_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "pan_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "pan_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "pan_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "pan_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "pan_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0_a);
        region.parseOpcode({ "pan_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 200.0_a);
        region.parseOpcode({ "pan_stepcc120", "-2" });
        REQUIRE(view.at(120).step == 0.0f);
    }

    SECTION("width")
    {
        REQUIRE(region.width == 1.0_a);
        region.parseOpcode({ "width", "4.2" });
        REQUIRE(region.width == 0.042_a);
        region.parseOpcode({ "width", "-4.2" });
        REQUIRE(region.width == -0.042_a);
        region.parseOpcode({ "width", "-123" });
        REQUIRE(region.width == -1.0_a);
        region.parseOpcode({ "width", "132" });
        REQUIRE(region.width == 1.0_a);
    }

    SECTION("width_oncc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Width, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "width_oncc45", "4.2" });
        REQUIRE(view.at(45).value == 4.2_a);
        region.parseOpcode({ "width_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "width_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "width_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "width_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "width_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "width_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "width_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0_a);
        region.parseOpcode({ "width_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 200.0_a);
        region.parseOpcode({ "width_stepcc120", "-20" });
        REQUIRE(view.at(120).step == 0.0f);
    }

    SECTION("position")
    {
        REQUIRE(region.position == 0.0f);
        region.parseOpcode({ "position", "4.2" });
        REQUIRE(region.position == 0.042_a);
        region.parseOpcode({ "position", "-4.2" });
        REQUIRE(region.position == -0.042_a);
        region.parseOpcode({ "position", "-123" });
        REQUIRE(region.position == -1.0_a);
        region.parseOpcode({ "position", "132" });
        REQUIRE(region.position == 1.0_a);
    }

    SECTION("position_oncc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Position, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "position_oncc45", "4.2" });
        REQUIRE(view.at(45).value == 4.2_a);
        region.parseOpcode({ "position_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "position_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "position_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "position_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "position_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "position_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "position_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0_a);
        region.parseOpcode({ "position_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 200.0_a);
        region.parseOpcode({ "position_stepcc120", "-2" });
        REQUIRE(view.at(120).step == 0.0f);
    }

    SECTION("amp_keycenter")
    {
        REQUIRE(region.ampKeycenter == 60);
        region.parseOpcode({ "amp_keycenter", "40" });
        REQUIRE(region.ampKeycenter == 40);
        region.parseOpcode({ "amp_keycenter", "-1" });
        REQUIRE(region.ampKeycenter == 0);
        region.parseOpcode({ "amp_keycenter", "132" });
        REQUIRE(region.ampKeycenter == 127);
    }

    SECTION("amp_keytrack")
    {
        REQUIRE(region.ampKeytrack == 0.0f);
        region.parseOpcode({ "amp_keytrack", "4.2" });
        REQUIRE(region.ampKeytrack == 4.2f);
        region.parseOpcode({ "amp_keytrack", "-4.2" });
        REQUIRE(region.ampKeytrack == -4.2f);
        region.parseOpcode({ "amp_keytrack", "-123" });
        REQUIRE(region.ampKeytrack == -96.0f);
        region.parseOpcode({ "amp_keytrack", "132" });
        REQUIRE(region.ampKeytrack == 12.0f);
    }

    SECTION("amp_veltrack")
    {
        REQUIRE(region.ampVeltrack == 100.0f);
        region.parseOpcode({ "amp_veltrack", "4.2" });
        REQUIRE(region.ampVeltrack == 4.2f);
        region.parseOpcode({ "amp_veltrack", "-4.2" });
        REQUIRE(region.ampVeltrack == -4.2f);
        region.parseOpcode({ "amp_veltrack", "-123" });
        REQUIRE(region.ampVeltrack == -100.0f);
        region.parseOpcode({ "amp_veltrack", "132" });
        REQUIRE(region.ampVeltrack == 100.0f);
    }

    SECTION("amp_random")
    {
        REQUIRE(region.ampRandom == 0.0f);
        region.parseOpcode({ "amp_random", "4.2" });
        REQUIRE(region.ampRandom == 4.2f);
        region.parseOpcode({ "amp_random", "-4.2" });
        REQUIRE(region.ampRandom == 0.0f);
        region.parseOpcode({ "amp_random", "132" });
        REQUIRE(region.ampRandom == 24.0f);
    }

    SECTION("amp_velcurve")
    {
        region.parseOpcode({ "amp_velcurve_6", "0.4" });
        REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(6, 0.4f));
        region.parseOpcode({ "amp_velcurve_127", "-1.0" });
        REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(127, 0.0f));
        region.parseOpcode({ "amp_velcurve_008", "0.3" });
        REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(8, 0.3f));
        region.parseOpcode({ "amp_velcurve_064", "0.9" });
        REQUIRE(region.velocityPoints.back() == std::pair<uint8_t, float>(64, 0.9f));
    }

    SECTION("xfin_lokey, xfin_hikey")
    {
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(0, 0));
        region.parseOpcode({ "xfin_lokey", "4" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(4, 4));
        region.parseOpcode({ "xfin_lokey", "128" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "xfin_lokey", "59" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(59, 127));
        region.parseOpcode({ "xfin_hikey", "59" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(59, 59));
        region.parseOpcode({ "xfin_hikey", "128" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(59, 127));
        region.parseOpcode({ "xfin_hikey", "0" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(0, 0));
        region.parseOpcode({ "xfin_hikey", "-1" });
        REQUIRE(region.crossfadeKeyInRange == Range<uint8_t>(0, 0));
    }

    SECTION("xfin_lovel, xfin_hivel")
    {
        REQUIRE(region.crossfadeVelInRange == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "xfin_lovel", "4" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(4_norm, 4_norm));
        region.parseOpcode({ "xfin_lovel", "128" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "xfin_lovel", "59" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfin_hivel", "59" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(59_norm, 59_norm));
        region.parseOpcode({ "xfin_hivel", "128" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfin_hivel", "0" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "xfin_hivel", "-1" });
        REQUIRE(region.crossfadeVelInRange == Range<float>(0_norm, 0_norm));
    }

    SECTION("xfout_lokey, xfout_hikey")
    {
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "xfout_lokey", "4" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(4, 127));
        region.parseOpcode({ "xfout_lokey", "128" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(127, 127));
        region.parseOpcode({ "xfout_lokey", "59" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(59, 127));
        region.parseOpcode({ "xfout_hikey", "59" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(59, 59));
        region.parseOpcode({ "xfout_hikey", "128" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(59, 127));
        region.parseOpcode({ "xfout_hikey", "0" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(0, 0));
        region.parseOpcode({ "xfout_hikey", "-1" });
        REQUIRE(region.crossfadeKeyOutRange == Range<uint8_t>(0, 0));
    }

    SECTION("xfout_lovel, xfout_hivel")
    {
        REQUIRE(region.crossfadeVelOutRange == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "xfout_lovel", "4" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(4_norm, 127_norm));
        region.parseOpcode({ "xfout_lovel", "128" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "xfout_lovel", "59" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfout_hivel", "59" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(59_norm, 59_norm));
        region.parseOpcode({ "xfout_hivel", "128" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfout_hivel", "0" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "xfout_hivel", "-1" });
        REQUIRE(region.crossfadeVelOutRange == Range<float>(0_norm, 0_norm));
    }

    SECTION("xfin_locc, xfin_hicc")
    {
        REQUIRE(!region.crossfadeCCInRange.contains(4));
        region.parseOpcode({ "xfin_locc4", "4" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(4_norm, 4_norm));
        region.parseOpcode({ "xfin_locc4", "128" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "xfin_locc4", "59" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfin_hicc4", "59" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(59_norm, 59_norm));
        region.parseOpcode({ "xfin_hicc4", "128" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfin_hicc4", "0" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "xfin_hicc4", "-1" });
        REQUIRE(region.crossfadeCCInRange[4] == Range<float>(0_norm, 0_norm));
    }

    SECTION("xfout_locc, xfout_hicc")
    {
        REQUIRE(!region.crossfadeCCOutRange.contains(4));
        region.parseOpcode({ "xfout_locc4", "4" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(4_norm, 127_norm));
        region.parseOpcode({ "xfout_locc4", "128" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(127_norm, 127_norm));
        region.parseOpcode({ "xfout_locc4", "59" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfout_hicc4", "59" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(59_norm, 59_norm));
        region.parseOpcode({ "xfout_hicc4", "128" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(59_norm, 127_norm));
        region.parseOpcode({ "xfout_hicc4", "0" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(0_norm, 0_norm));
        region.parseOpcode({ "xfout_hicc4", "-1" });
        REQUIRE(region.crossfadeCCOutRange[4] == Range<float>(0_norm, 0_norm));
    }

    SECTION("xf_keycurve")
    {
        REQUIRE(region.crossfadeKeyCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_keycurve", "gain" });
        REQUIRE(region.crossfadeKeyCurve == SfzCrossfadeCurve::gain);
        region.parseOpcode({ "xf_keycurve", "power" });
        REQUIRE(region.crossfadeKeyCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_keycurve", "something" });
        REQUIRE(region.crossfadeKeyCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_keycurve", "gain" });
        region.parseOpcode({ "xf_keycurve", "something" });
        REQUIRE(region.crossfadeKeyCurve == SfzCrossfadeCurve::gain);
    }

    SECTION("xf_velcurve")
    {
        REQUIRE(region.crossfadeVelCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_velcurve", "gain" });
        REQUIRE(region.crossfadeVelCurve == SfzCrossfadeCurve::gain);
        region.parseOpcode({ "xf_velcurve", "power" });
        REQUIRE(region.crossfadeVelCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_velcurve", "something" });
        REQUIRE(region.crossfadeVelCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_velcurve", "gain" });
        region.parseOpcode({ "xf_velcurve", "something" });
        REQUIRE(region.crossfadeVelCurve == SfzCrossfadeCurve::gain);
    }

    SECTION("xf_cccurve")
    {
        REQUIRE(region.crossfadeCCCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_cccurve", "gain" });
        REQUIRE(region.crossfadeCCCurve == SfzCrossfadeCurve::gain);
        region.parseOpcode({ "xf_cccurve", "power" });
        REQUIRE(region.crossfadeCCCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_cccurve", "something" });
        REQUIRE(region.crossfadeCCCurve == SfzCrossfadeCurve::power);
        region.parseOpcode({ "xf_cccurve", "gain" });
        region.parseOpcode({ "xf_cccurve", "something" });
        REQUIRE(region.crossfadeCCCurve == SfzCrossfadeCurve::gain);
    }

    SECTION("pitch_keycenter")
    {
        REQUIRE(region.pitchKeycenter == 60);
        region.parseOpcode({ "pitch_keycenter", "40" });
        REQUIRE(region.pitchKeycenter == 40);
        region.parseOpcode({ "pitch_keycenter", "-1" });
        REQUIRE(region.pitchKeycenter == 0);
        region.parseOpcode({ "pitch_keycenter", "132" });
        REQUIRE(region.pitchKeycenter == 127);
    }

    SECTION("pitch_keytrack")
    {
        REQUIRE(region.pitchKeytrack == 100);
        region.parseOpcode({ "pitch_keytrack", "40" });
        REQUIRE(region.pitchKeytrack == 40);
        region.parseOpcode({ "pitch_keytrack", "-1" });
        REQUIRE(region.pitchKeytrack == -1);
        region.parseOpcode({ "pitch_keytrack", "1320" });
        REQUIRE(region.pitchKeytrack == 1200);
        region.parseOpcode({ "pitch_keytrack", "-1320" });
        REQUIRE(region.pitchKeytrack == -1200);
    }

    SECTION("pitch_random")
    {
        REQUIRE(region.pitchRandom == 0);
        region.parseOpcode({ "pitch_random", "40" });
        REQUIRE(region.pitchRandom == 40);
        region.parseOpcode({ "pitch_random", "-1" });
        REQUIRE(region.pitchRandom == 0);
        region.parseOpcode({ "pitch_random", "10320" });
        REQUIRE(region.pitchRandom == 9600);
    }

    SECTION("pitch_veltrack")
    {
        REQUIRE(region.pitchVeltrack == 0);
        region.parseOpcode({ "pitch_veltrack", "40" });
        REQUIRE(region.pitchVeltrack == 40);
        region.parseOpcode({ "pitch_veltrack", "-1" });
        REQUIRE(region.pitchVeltrack == -1);
        region.parseOpcode({ "pitch_veltrack", "13020" });
        REQUIRE(region.pitchVeltrack == 9600);
        region.parseOpcode({ "pitch_veltrack", "-13020" });
        REQUIRE(region.pitchVeltrack == -9600);
    }

    SECTION("transpose")
    {
        REQUIRE(region.transpose == 0);
        region.parseOpcode({ "transpose", "40" });
        REQUIRE(region.transpose == 40);
        region.parseOpcode({ "transpose", "-1" });
        REQUIRE(region.transpose == -1);
        region.parseOpcode({ "transpose", "154" });
        REQUIRE(region.transpose == 127);
        region.parseOpcode({ "transpose", "-154" });
        REQUIRE(region.transpose == -127);
    }

    SECTION("tune")
    {
        REQUIRE(region.tune == 0);
        region.parseOpcode({ "tune", "40" });
        REQUIRE(region.tune == 40);
        region.parseOpcode({ "tune", "-1" });
        REQUIRE(region.tune == -1);
        region.parseOpcode({ "tune", "15432" });
        REQUIRE(region.tune == 9600);
        region.parseOpcode({ "tune", "-15432" });
        REQUIRE(region.tune == -9600);
    }

    SECTION("bend_up, bend_down, bend_step, bend_smooth")
    {
        REQUIRE(region.bendUp == 200);
        REQUIRE(region.bendDown == -200);
        REQUIRE(region.bendStep == 1);
        region.parseOpcode({ "bend_up", "400" });
        REQUIRE(region.bendUp == 400);
        region.parseOpcode({ "bend_up", "-200" });
        REQUIRE(region.bendUp == -200);
        region.parseOpcode({ "bend_up", "9700" });
        REQUIRE(region.bendUp == 9600);
        region.parseOpcode({ "bend_up", "-9700" });
        REQUIRE(region.bendUp == -9600);
        region.parseOpcode({ "bend_down", "400" });
        REQUIRE(region.bendDown == 400);
        region.parseOpcode({ "bend_down", "-200" });
        REQUIRE(region.bendDown == -200);
        region.parseOpcode({ "bend_down", "9700" });
        REQUIRE(region.bendDown == 9600);
        region.parseOpcode({ "bend_down", "-9700" });
        REQUIRE(region.bendDown == -9600);
        region.parseOpcode({ "bend_step", "400" });
        REQUIRE(region.bendStep == 400);
        region.parseOpcode({ "bend_step", "-200" });
        REQUIRE(region.bendStep == 1);
        region.parseOpcode({ "bend_step", "9700" });
        REQUIRE(region.bendStep == 1200);
        region.parseOpcode({ "bend_smooth", "10" });
        REQUIRE(region.bendSmooth == 10);
        region.parseOpcode({ "bend_smooth", "120" });
        REQUIRE(region.bendSmooth == 100);
        region.parseOpcode({ "bend_smooth", "-2" });
        REQUIRE(region.bendSmooth == 0);
    }

    SECTION("ampeg")
    {
        // Defaults
        REQUIRE(region.amplitudeEG.attack == 0.0f);
        REQUIRE(region.amplitudeEG.decay == 0.0f);
        REQUIRE(region.amplitudeEG.delay == 0.0f);
        REQUIRE(region.amplitudeEG.hold == 0.0f);
        REQUIRE(region.amplitudeEG.release == 0.0f);
        REQUIRE(region.amplitudeEG.start == 0.0f);
        REQUIRE(region.amplitudeEG.sustain == 100.0f);
        REQUIRE(region.amplitudeEG.depth == 0);
        REQUIRE(region.amplitudeEG.vel2attack == 0.0f);
        REQUIRE(region.amplitudeEG.vel2decay == 0.0f);
        REQUIRE(region.amplitudeEG.vel2delay == 0.0f);
        REQUIRE(region.amplitudeEG.vel2hold == 0.0f);
        REQUIRE(region.amplitudeEG.vel2release == 0.0f);
        REQUIRE(region.amplitudeEG.vel2sustain == 0.0f);
        REQUIRE(region.amplitudeEG.vel2depth == 0);
        //
        region.parseOpcode({ "ampeg_attack", "1" });
        region.parseOpcode({ "ampeg_decay", "2" });
        region.parseOpcode({ "ampeg_delay", "3" });
        region.parseOpcode({ "ampeg_hold", "4" });
        region.parseOpcode({ "ampeg_release", "5" });
        region.parseOpcode({ "ampeg_start", "6" });
        region.parseOpcode({ "ampeg_sustain", "7" });
        region.parseOpcode({ "ampeg_depth", "8" });
        region.parseOpcode({ "ampeg_vel2attack", "9" });
        region.parseOpcode({ "ampeg_vel2decay", "10" });
        region.parseOpcode({ "ampeg_vel2delay", "11" });
        region.parseOpcode({ "ampeg_vel2hold", "12" });
        region.parseOpcode({ "ampeg_vel2release", "13" });
        region.parseOpcode({ "ampeg_vel2sustain", "14" });
        region.parseOpcode({ "ampeg_vel2depth", "15" });
        REQUIRE(region.amplitudeEG.attack == 1.0f);
        REQUIRE(region.amplitudeEG.decay == 2.0f);
        REQUIRE(region.amplitudeEG.delay == 3.0f);
        REQUIRE(region.amplitudeEG.hold == 4.0f);
        REQUIRE(region.amplitudeEG.release == 5.0f);
        REQUIRE(region.amplitudeEG.start == 6.0f);
        REQUIRE(region.amplitudeEG.sustain == 7.0f);
        REQUIRE(region.amplitudeEG.depth == 0); // ignored for ampeg
        REQUIRE(region.amplitudeEG.vel2attack == 9.0f);
        REQUIRE(region.amplitudeEG.vel2decay == 10.0f);
        REQUIRE(region.amplitudeEG.vel2delay == 11.0f);
        REQUIRE(region.amplitudeEG.vel2hold == 12.0f);
        REQUIRE(region.amplitudeEG.vel2release == 13.0f);
        REQUIRE(region.amplitudeEG.vel2sustain == 14.0f);
        REQUIRE(region.amplitudeEG.vel2depth == 0); // ignored for ampeg
        //
        region.parseOpcode({ "ampeg_attack", "1000" });
        region.parseOpcode({ "ampeg_decay", "1000" });
        region.parseOpcode({ "ampeg_delay", "1000" });
        region.parseOpcode({ "ampeg_hold", "1000" });
        region.parseOpcode({ "ampeg_release", "1000" });
        region.parseOpcode({ "ampeg_start", "1000" });
        region.parseOpcode({ "ampeg_sustain", "1000" });
        region.parseOpcode({ "ampeg_depth", "1000" });
        region.parseOpcode({ "ampeg_vel2attack", "1000" });
        region.parseOpcode({ "ampeg_vel2decay", "1000" });
        region.parseOpcode({ "ampeg_vel2delay", "1000" });
        region.parseOpcode({ "ampeg_vel2hold", "1000" });
        region.parseOpcode({ "ampeg_vel2release", "1000" });
        region.parseOpcode({ "ampeg_vel2sustain", "1000" });
        region.parseOpcode({ "ampeg_vel2depth", "1000" });
        REQUIRE(region.amplitudeEG.attack == 100.0f);
        REQUIRE(region.amplitudeEG.decay == 100.0f);
        REQUIRE(region.amplitudeEG.delay == 100.0f);
        REQUIRE(region.amplitudeEG.hold == 100.0f);
        REQUIRE(region.amplitudeEG.release == 100.0f);
        REQUIRE(region.amplitudeEG.start == 100.0f);
        REQUIRE(region.amplitudeEG.sustain == 100.0f);
        REQUIRE(region.amplitudeEG.depth == 0); // ignored for ampeg
        REQUIRE(region.amplitudeEG.vel2attack == 100.0f);
        REQUIRE(region.amplitudeEG.vel2decay == 100.0f);
        REQUIRE(region.amplitudeEG.vel2delay == 100.0f);
        REQUIRE(region.amplitudeEG.vel2hold == 100.0f);
        REQUIRE(region.amplitudeEG.vel2release == 100.0f);
        REQUIRE(region.amplitudeEG.vel2sustain == 100.0f);
        REQUIRE(region.amplitudeEG.vel2depth == 0); // ignored for ampeg
        //
        region.parseOpcode({ "ampeg_attack", "-101" });
        region.parseOpcode({ "ampeg_decay", "-101" });
        region.parseOpcode({ "ampeg_delay", "-101" });
        region.parseOpcode({ "ampeg_hold", "-101" });
        region.parseOpcode({ "ampeg_release", "-101" });
        region.parseOpcode({ "ampeg_start", "-101" });
        region.parseOpcode({ "ampeg_sustain", "-101" });
        region.parseOpcode({ "ampeg_depth", "-101" });
        region.parseOpcode({ "ampeg_vel2attack", "-101" });
        region.parseOpcode({ "ampeg_vel2decay", "-101" });
        region.parseOpcode({ "ampeg_vel2delay", "-101" });
        region.parseOpcode({ "ampeg_vel2hold", "-101" });
        region.parseOpcode({ "ampeg_vel2release", "-101" });
        region.parseOpcode({ "ampeg_vel2sustain", "-101" });
        region.parseOpcode({ "ampeg_vel2depth", "-101" });
        REQUIRE(region.amplitudeEG.attack == 0.0f);
        REQUIRE(region.amplitudeEG.decay == 0.0f);
        REQUIRE(region.amplitudeEG.delay == 0.0f);
        REQUIRE(region.amplitudeEG.hold == 0.0f);
        REQUIRE(region.amplitudeEG.release == 0.0f);
        REQUIRE(region.amplitudeEG.start == 0.0f);
        REQUIRE(region.amplitudeEG.sustain == 0.0f);
        REQUIRE(region.amplitudeEG.depth == 0); // ignored for ampeg
        REQUIRE(region.amplitudeEG.vel2attack == -100.0f);
        REQUIRE(region.amplitudeEG.vel2decay == -100.0f);
        REQUIRE(region.amplitudeEG.vel2delay == -100.0f);
        REQUIRE(region.amplitudeEG.vel2hold == -100.0f);
        REQUIRE(region.amplitudeEG.vel2release == -100.0f);
        REQUIRE(region.amplitudeEG.vel2sustain == -100.0f);
    }

    SECTION("ampeg_XX_onccNN")
    {
        // Defaults
        REQUIRE(region.amplitudeEG.ccAttack.empty());
        REQUIRE(region.amplitudeEG.ccDecay.empty());
        REQUIRE(region.amplitudeEG.ccDelay.empty());
        REQUIRE(region.amplitudeEG.ccHold.empty());
        REQUIRE(region.amplitudeEG.ccRelease.empty());
        REQUIRE(region.amplitudeEG.ccStart.empty());
        REQUIRE(region.amplitudeEG.ccSustain.empty());
        //
        region.parseOpcode({ "ampeg_attack_oncc1", "1" });
        region.parseOpcode({ "ampeg_decay_oncc2", "2" });
        region.parseOpcode({ "ampeg_delay_oncc3", "3" });
        region.parseOpcode({ "ampeg_hold_oncc4", "4" });
        region.parseOpcode({ "ampeg_release_oncc5", "5" });
        region.parseOpcode({ "ampeg_start_oncc6", "6" });
        region.parseOpcode({ "ampeg_sustain_oncc7", "7" });
        REQUIRE(region.amplitudeEG.ccAttack.contains(1));
        REQUIRE(region.amplitudeEG.ccDecay.contains(2));
        REQUIRE(region.amplitudeEG.ccDelay.contains(3));
        REQUIRE(region.amplitudeEG.ccHold.contains(4));
        REQUIRE(region.amplitudeEG.ccRelease.contains(5));
        REQUIRE(region.amplitudeEG.ccStart.contains(6));
        REQUIRE(region.amplitudeEG.ccSustain.contains(7));
        REQUIRE(region.amplitudeEG.ccAttack[1] == 1.0f);
        REQUIRE(region.amplitudeEG.ccDecay[2] == 2.0f);
        REQUIRE(region.amplitudeEG.ccDelay[3] == 3.0f);
        REQUIRE(region.amplitudeEG.ccHold[4] == 4.0f);
        REQUIRE(region.amplitudeEG.ccRelease[5] == 5.0f);
        REQUIRE(region.amplitudeEG.ccStart[6] == 6.0f);
        REQUIRE(region.amplitudeEG.ccSustain[7] == 7.0f);
        //
        region.parseOpcode({ "ampeg_attack_oncc1", "101" });
        region.parseOpcode({ "ampeg_decay_oncc2", "101" });
        region.parseOpcode({ "ampeg_delay_oncc3", "101" });
        region.parseOpcode({ "ampeg_hold_oncc4", "101" });
        region.parseOpcode({ "ampeg_release_oncc5", "101" });
        region.parseOpcode({ "ampeg_start_oncc6", "101" });
        region.parseOpcode({ "ampeg_sustain_oncc7", "101" });
        REQUIRE(region.amplitudeEG.ccAttack[1] == 100.0f);
        REQUIRE(region.amplitudeEG.ccDecay[2] == 100.0f);
        REQUIRE(region.amplitudeEG.ccDelay[3] == 100.0f);
        REQUIRE(region.amplitudeEG.ccHold[4] == 100.0f);
        REQUIRE(region.amplitudeEG.ccRelease[5] == 100.0f);
        REQUIRE(region.amplitudeEG.ccStart[6] == 100.0f);
        REQUIRE(region.amplitudeEG.ccSustain[7] == 100.0f);
        //
        region.parseOpcode({ "ampeg_attack_oncc1", "-101" });
        region.parseOpcode({ "ampeg_decay_oncc2", "-101" });
        region.parseOpcode({ "ampeg_delay_oncc3", "-101" });
        region.parseOpcode({ "ampeg_hold_oncc4", "-101" });
        region.parseOpcode({ "ampeg_release_oncc5", "-101" });
        region.parseOpcode({ "ampeg_start_oncc6", "-101" });
        region.parseOpcode({ "ampeg_sustain_oncc7", "-101" });
        REQUIRE(region.amplitudeEG.ccAttack[1] == -100.0f);
        REQUIRE(region.amplitudeEG.ccDecay[2] == -100.0f);
        REQUIRE(region.amplitudeEG.ccDelay[3] == -100.0f);
        REQUIRE(region.amplitudeEG.ccHold[4] == -100.0f);
        REQUIRE(region.amplitudeEG.ccRelease[5] == -100.0f);
        REQUIRE(region.amplitudeEG.ccStart[6] == -100.0f);
        REQUIRE(region.amplitudeEG.ccSustain[7] == -100.0f);
        //
        region.parseOpcode({ "ampeg_attack_oncc1", "1" });
        region.parseOpcode({ "ampeg_decay_oncc2", "2" });
        region.parseOpcode({ "ampeg_delay_oncc3", "3" });
        region.parseOpcode({ "ampeg_hold_oncc4", "4" });
        region.parseOpcode({ "ampeg_release_oncc5", "5" });
        region.parseOpcode({ "ampeg_start_oncc6", "6" });
        region.parseOpcode({ "ampeg_sustain_oncc7", "7" });
        region.parseOpcode({ "ampeg_attack_oncc2", "2" });
        region.parseOpcode({ "ampeg_decay_oncc3", "3" });
        region.parseOpcode({ "ampeg_delay_oncc4", "4" });
        region.parseOpcode({ "ampeg_hold_oncc5", "5" });
        region.parseOpcode({ "ampeg_release_oncc6", "6" });
        region.parseOpcode({ "ampeg_start_oncc7", "7" });
        region.parseOpcode({ "ampeg_sustain_oncc8", "8" });
        REQUIRE(region.amplitudeEG.ccAttack.contains(1));
        REQUIRE(region.amplitudeEG.ccDecay.contains(2));
        REQUIRE(region.amplitudeEG.ccDelay.contains(3));
        REQUIRE(region.amplitudeEG.ccHold.contains(4));
        REQUIRE(region.amplitudeEG.ccRelease.contains(5));
        REQUIRE(region.amplitudeEG.ccStart.contains(6));
        REQUIRE(region.amplitudeEG.ccSustain.contains(7));
        REQUIRE(region.amplitudeEG.ccAttack.contains(2));
        REQUIRE(region.amplitudeEG.ccDecay.contains(3));
        REQUIRE(region.amplitudeEG.ccDelay.contains(4));
        REQUIRE(region.amplitudeEG.ccHold.contains(5));
        REQUIRE(region.amplitudeEG.ccRelease.contains(6));
        REQUIRE(region.amplitudeEG.ccStart.contains(7));
        REQUIRE(region.amplitudeEG.ccSustain.contains(8));
        REQUIRE(region.amplitudeEG.ccAttack[1] == 1.0f);
        REQUIRE(region.amplitudeEG.ccDecay[2] == 2.0f);
        REQUIRE(region.amplitudeEG.ccDelay[3] == 3.0f);
        REQUIRE(region.amplitudeEG.ccHold[4] == 4.0f);
        REQUIRE(region.amplitudeEG.ccRelease[5] == 5.0f);
        REQUIRE(region.amplitudeEG.ccStart[6] == 6.0f);
        REQUIRE(region.amplitudeEG.ccSustain[7] == 7.0f);
        REQUIRE(region.amplitudeEG.ccAttack[2] == 2.0f);
        REQUIRE(region.amplitudeEG.ccDecay[3] == 3.0f);
        REQUIRE(region.amplitudeEG.ccDelay[4] == 4.0f);
        REQUIRE(region.amplitudeEG.ccHold[5] == 5.0f);
        REQUIRE(region.amplitudeEG.ccRelease[6] == 6.0f);
        REQUIRE(region.amplitudeEG.ccStart[7] == 7.0f);
        REQUIRE(region.amplitudeEG.ccSustain[8] == 8.0f);
    }

    SECTION("sustain_sw and sostenuto_sw")
    {
        REQUIRE(region.checkSustain);
        REQUIRE(region.checkSostenuto);
        region.parseOpcode({ "sustain_sw", "off" });
        REQUIRE(!region.checkSustain);
        region.parseOpcode({ "sustain_sw", "on" });
        REQUIRE(region.checkSustain);
        region.parseOpcode({ "sustain_sw", "off" });
        region.parseOpcode({ "sustain_sw", "obladi" });
        REQUIRE(region.checkSustain);
        region.parseOpcode({ "sostenuto_sw", "off" });
        REQUIRE(!region.checkSostenuto);
        region.parseOpcode({ "sostenuto_sw", "on" });
        REQUIRE(region.checkSostenuto);
        region.parseOpcode({ "sostenuto_sw", "off" });
        region.parseOpcode({ "sostenuto_sw", "obladi" });
        REQUIRE(region.checkSostenuto);
    }

    SECTION("sustain_cc")
    {
        REQUIRE(region.sustainCC == 64);
        region.parseOpcode({ "sustain_cc", "63" });
        REQUIRE(region.sustainCC == 63);
        region.parseOpcode({ "sustain_cc", "-1" });
        REQUIRE(region.sustainCC == 0);
    }

    SECTION("sustain_lo")
    {
        REQUIRE(region.sustainThreshold == Approx(0.5_norm).margin(1e-3));
        region.parseOpcode({ "sustain_lo", "-1" });
        REQUIRE(region.sustainThreshold == 0_norm);
        region.parseOpcode({ "sustain_lo", "1" });
        REQUIRE(region.sustainThreshold == 1_norm);
        region.parseOpcode({ "sustain_lo", "63" });
        REQUIRE(region.sustainThreshold == 63_norm);
        region.parseOpcode({ "sustain_lo", "128" });
        REQUIRE(region.sustainThreshold == 127_norm);
    }

    SECTION("Filter stacking and cutoffs")
    {
        REQUIRE(region.filters.empty());

        region.parseOpcode({ "cutoff", "500" });
        REQUIRE(region.filters.size() == 1);
        REQUIRE(region.filters[0].cutoff == 500.0f);
        // Check filter defaults
        REQUIRE(region.filters[0].keycenter == 60);
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf2p);
        REQUIRE(region.filters[0].keytrack == 0);
        REQUIRE(region.filters[0].gain == 0);
        REQUIRE(region.filters[0].veltrack == 0);
        REQUIRE(region.filters[0].resonance == 0.0f);
        REQUIRE(region.filters[0].cutoffCC.empty());
        REQUIRE(region.filters[0].gainCC.empty());
        REQUIRE(region.filters[0].resonanceCC.empty());

        region.parseOpcode({ "cutoff2", "5000" });
        REQUIRE(region.filters.size() == 2);
        REQUIRE(region.filters[1].cutoff == 5000.0f);
        // Check filter defaults
        REQUIRE(region.filters[1].keycenter == 60);
        REQUIRE(region.filters[1].type == FilterType::kFilterLpf2p);
        REQUIRE(region.filters[1].keytrack == 0);
        REQUIRE(region.filters[1].gain == 0);
        REQUIRE(region.filters[1].veltrack == 0);
        REQUIRE(region.filters[1].resonance == 0.0f);
        REQUIRE(region.filters[1].cutoffCC.empty());
        REQUIRE(region.filters[1].gainCC.empty());
        REQUIRE(region.filters[1].resonanceCC.empty());

        region.parseOpcode({ "cutoff4", "50" });
        REQUIRE(region.filters.size() == 4);
        REQUIRE(region.filters[2].cutoff == 0.0f);
        REQUIRE(region.filters[3].cutoff == 50.0f);
        // Check filter defaults
        REQUIRE(region.filters[2].keycenter == 60);
        REQUIRE(region.filters[2].type == FilterType::kFilterLpf2p);
        REQUIRE(region.filters[2].keytrack == 0);
        REQUIRE(region.filters[2].gain == 0);
        REQUIRE(region.filters[2].veltrack == 0);
        REQUIRE(region.filters[2].resonance == 0.0f);
        REQUIRE(region.filters[2].cutoffCC.empty());
        REQUIRE(region.filters[2].gainCC.empty());
        REQUIRE(region.filters[2].resonanceCC.empty());
        REQUIRE(region.filters[3].keycenter == 60);
        REQUIRE(region.filters[3].type == FilterType::kFilterLpf2p);
        REQUIRE(region.filters[3].keytrack == 0);
        REQUIRE(region.filters[3].gain == 0);
        REQUIRE(region.filters[3].veltrack == 0);
        REQUIRE(region.filters[3].resonance == 0.0f);
        REQUIRE(region.filters[3].cutoffCC.empty());
        REQUIRE(region.filters[3].gainCC.empty());
        REQUIRE(region.filters[3].resonanceCC.empty());
    }

    SECTION("Filter parameter dispatch")
    {
        region.parseOpcode({ "cutoff3", "50" });
        REQUIRE(region.filters.size() == 3);
        REQUIRE(region.filters[2].cutoff == 50.0f);
        region.parseOpcode({ "resonance2", "3" });
        REQUIRE(region.filters[1].resonance == 3.0f);
        region.parseOpcode({ "fil2_gain", "-5" });
        REQUIRE(region.filters[1].gain == -5.0f);
        region.parseOpcode({ "fil_gain", "5" });
        REQUIRE(region.filters[0].gain == 5.0f);
        region.parseOpcode({ "fil1_gain", "-5" });
        REQUIRE(region.filters[0].gain == -5.0f);
        region.parseOpcode({ "fil2_veltrack", "-100" });
        REQUIRE(region.filters[1].veltrack == -100);
        region.parseOpcode({ "fil3_keytrack", "100" });
        REQUIRE(region.filters[2].keytrack == 100);
        REQUIRE(region.filters[0].cutoffCC.empty());
        region.parseOpcode({ "cutoff1_cc15", "210" });
        REQUIRE(region.filters[0].cutoffCC.contains(15));
        REQUIRE(region.filters[0].cutoffCC[15] == 210);
        region.parseOpcode({ "resonance3_cc24", "10" });
        REQUIRE(region.filters[2].resonanceCC.contains(24));
        REQUIRE(region.filters[2].resonanceCC[24] == 10);
        region.parseOpcode({ "fil2_gain_oncc12", "-50" });
        REQUIRE(region.filters[1].gainCC.contains(12));
        REQUIRE(region.filters[1].gainCC[12] == -50.0f);

    }

    SECTION("Filter values")
    {
        REQUIRE(region.filters.empty());

        region.parseOpcode({ "cutoff", "500" });
        REQUIRE(region.filters.size() == 1);
        REQUIRE(region.filters[0].cutoff == 500.0f);
        region.parseOpcode({ "cutoff", "-100" });
        REQUIRE(region.filters[0].cutoff == 0.0f);
        region.parseOpcode({ "cutoff", "2000000" });
        REQUIRE(region.filters[0].cutoff == 20000.0f);

        REQUIRE(region.filters[0].resonance == 0.0f);
        region.parseOpcode({ "resonance", "5" });
        REQUIRE(region.filters[0].resonance == 5.0f);
        region.parseOpcode({ "resonance", "-5" });
        REQUIRE(region.filters[0].resonance == 0.0f);
        region.parseOpcode({ "resonance", "500" });
        REQUIRE(region.filters[0].resonance == 96.0f);

        REQUIRE(region.filters[0].veltrack == 0);
        region.parseOpcode({ "fil_veltrack", "50" });
        REQUIRE(region.filters[0].veltrack == 50);
        region.parseOpcode({ "fil_veltrack", "-5" });
        REQUIRE(region.filters[0].veltrack == -5);
        region.parseOpcode({ "fil_veltrack", "10000" });
        REQUIRE(region.filters[0].veltrack == 9600);
        region.parseOpcode({ "fil_veltrack", "-10000" });
        REQUIRE(region.filters[0].veltrack == -9600);

        REQUIRE(region.filters[0].keycenter == 60);
        region.parseOpcode({ "fil_keycenter", "50" });
        REQUIRE(region.filters[0].keycenter == 50);
        region.parseOpcode({ "fil_keycenter", "-2" });
        REQUIRE(region.filters[0].keycenter == 0);
        region.parseOpcode({ "fil_keycenter", "1000" });
        REQUIRE(region.filters[0].keycenter == 127);
        region.parseOpcode({ "fil_keycenter", "c4" });
        REQUIRE(region.filters[0].keycenter == 60);

        region.parseOpcode({ "fil_gain", "250" });
        REQUIRE(region.filters[0].gain == 96.0f);
        region.parseOpcode({ "fil_gain", "-200" });
        REQUIRE(region.filters[0].gain == -96.0f);

        region.parseOpcode({ "cutoff_cc43", "10000" });
        REQUIRE(region.filters[0].cutoffCC[43] == 9600);
        region.parseOpcode({ "cutoff_cc43", "-10000" });
        REQUIRE(region.filters[0].cutoffCC[43] == -9600);

        region.parseOpcode({ "resonance_cc43", "100" });
        REQUIRE(region.filters[0].resonanceCC[43] == 96.0f);
        region.parseOpcode({ "resonance_cc43", "-5" });
        REQUIRE(region.filters[0].resonanceCC[43] == 0.0f);
    }

    SECTION("Filter types")
    {
        REQUIRE(region.filters.empty());

        region.parseOpcode({ "fil_type", "lpf_1p" });
        REQUIRE(region.filters.size() == 1);
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf1p);
        region.parseOpcode({ "fil_type", "lpf_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf2p);
        region.parseOpcode({ "fil_type", "hpf_1p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHpf1p);
        region.parseOpcode({ "fil_type", "hpf_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHpf2p);
        region.parseOpcode({ "fil_type", "bpf_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBpf2p);
        region.parseOpcode({ "fil_type", "brf_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBrf2p);
        region.parseOpcode({ "fil_type", "bpf_1p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBpf1p);
        region.parseOpcode({ "fil_type", "brf_1p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBrf1p);
        region.parseOpcode({ "fil_type", "apf_1p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterApf1p);
        region.parseOpcode({ "fil_type", "lpf_2p_sv" });
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf2pSv);
        region.parseOpcode({ "fil_type", "hpf_2p_sv" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHpf2pSv);
        region.parseOpcode({ "fil_type", "bpf_2p_sv" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBpf2pSv);
        region.parseOpcode({ "fil_type", "brf_2p_sv" });
        REQUIRE(region.filters[0].type == FilterType::kFilterBrf2pSv);
        region.parseOpcode({ "fil_type", "lpf_4p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf4p);
        region.parseOpcode({ "fil_type", "hpf_4p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHpf4p);
        region.parseOpcode({ "fil_type", "lpf_6p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterLpf6p);
        region.parseOpcode({ "fil_type", "hpf_6p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHpf6p);
        region.parseOpcode({ "fil_type", "pink" });
        REQUIRE(region.filters[0].type == FilterType::kFilterPink);
        region.parseOpcode({ "fil_type", "lsh" });
        REQUIRE(region.filters[0].type == FilterType::kFilterLsh);
        region.parseOpcode({ "fil_type", "hsh" });
        REQUIRE(region.filters[0].type == FilterType::kFilterHsh);
        region.parseOpcode({ "fil_type", "peq" });
        REQUIRE(region.filters[0].type == FilterType::kFilterPeq);
        region.parseOpcode({ "fil_type", "lpf_1p" });
        region.parseOpcode({ "fil_type", "pkf_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterPeq);
        region.parseOpcode({ "fil_type", "lpf_1p" });
        region.parseOpcode({ "fil_type", "bpk_2p" });
        REQUIRE(region.filters[0].type == FilterType::kFilterPeq);
        region.parseOpcode({ "fil_type", "unknown" });
        REQUIRE(region.filters[0].type == FilterType::kFilterNone);
    }

    SECTION("EQ stacking and gains")
    {
        REQUIRE(region.equalizers.empty());

        region.parseOpcode({ "eq1_gain", "6" });
        REQUIRE(region.equalizers.size() == 1);
        REQUIRE(region.equalizers[0].gain == 6.0f);
        // Check defaults
        REQUIRE(region.equalizers[0].type == EqType::kEqPeak);
        REQUIRE(region.equalizers[0].bandwidth == 1.0f);
        REQUIRE(region.equalizers[0].frequency == 0.0f);
        REQUIRE(region.equalizers[0].vel2frequency == 0);
        REQUIRE(region.equalizers[0].vel2gain == 0);
        REQUIRE(region.equalizers[0].frequencyCC.empty());
        REQUIRE(region.equalizers[0].bandwidthCC.empty());
        REQUIRE(region.equalizers[0].gainCC.empty());

        region.parseOpcode({ "eq2_gain", "-400" });
        REQUIRE(region.equalizers.size() == 2);
        REQUIRE(region.equalizers[1].gain == -96.0f);
        // Check defaults
        REQUIRE(region.equalizers[1].type == EqType::kEqPeak);
        REQUIRE(region.equalizers[1].bandwidth == 1.0f);
        REQUIRE(region.equalizers[1].frequency == 0.0f);
        REQUIRE(region.equalizers[1].vel2frequency == 0);
        REQUIRE(region.equalizers[1].vel2gain == 0);
        REQUIRE(region.equalizers[1].frequencyCC.empty());
        REQUIRE(region.equalizers[1].bandwidthCC.empty());
        REQUIRE(region.equalizers[1].gainCC.empty());

        region.parseOpcode({ "eq4_gain", "500" });
        REQUIRE(region.equalizers.size() == 4);
        REQUIRE(region.equalizers[2].gain == 0.0f);
        REQUIRE(region.equalizers[3].type == EqType::kEqPeak);
        REQUIRE(region.equalizers[3].gain == 96.0f);
        // Check defaults
        REQUIRE(region.equalizers[2].bandwidth == 1.0f);
        REQUIRE(region.equalizers[2].frequency == 0.0f);
        REQUIRE(region.equalizers[2].vel2frequency == 0);
        REQUIRE(region.equalizers[2].vel2gain == 0);
        REQUIRE(region.equalizers[2].frequencyCC.empty());
        REQUIRE(region.equalizers[2].bandwidthCC.empty());
        REQUIRE(region.equalizers[2].gainCC.empty());
        REQUIRE(region.equalizers[3].bandwidth == 1.0f);
        REQUIRE(region.equalizers[3].frequency == 0.0f);
        REQUIRE(region.equalizers[3].vel2frequency == 0);
        REQUIRE(region.equalizers[3].vel2gain == 0);
        REQUIRE(region.equalizers[3].frequencyCC.empty());
        REQUIRE(region.equalizers[3].bandwidthCC.empty());
        REQUIRE(region.equalizers[3].gainCC.empty());
    }

    SECTION("EQ types")
    {
        region.parseOpcode({ "eq1_type", "hshelf" });
        REQUIRE(region.equalizers[0].type == EqType::kEqHighShelf);
        region.parseOpcode({ "eq1_type", "somethingsomething" });
        REQUIRE(region.equalizers[0].type == EqType::kEqNone);
        region.parseOpcode({ "eq1_type", "lshelf" });
        REQUIRE(region.equalizers[0].type == EqType::kEqLowShelf);
        region.parseOpcode({ "eq1_type", "peak" });
        REQUIRE(region.equalizers[0].type == EqType::kEqPeak);
    }

    SECTION("EQ parameter dispatch")
    {
        region.parseOpcode({ "eq3_bw", "2" });
        REQUIRE(region.equalizers.size() == 3);
        REQUIRE(region.equalizers[2].bandwidth == 2.0f);
        region.parseOpcode({ "eq1_gain", "-25" });
        REQUIRE(region.equalizers[0].gain == -25.0f);
        region.parseOpcode({ "eq2_freq", "300" });
        REQUIRE(region.equalizers[1].frequency == 300.0f);
        region.parseOpcode({ "eq3_type", "lshelf" });
        REQUIRE(region.equalizers[2].type == EqType::kEqLowShelf);
        region.parseOpcode({ "eq3_vel2gain", "10" });
        REQUIRE(region.equalizers[2].vel2gain == 10.0f);
        region.parseOpcode({ "eq1_vel2freq", "100" });
        REQUIRE(region.equalizers[0].vel2frequency == 100.0f);
        REQUIRE(region.equalizers[0].bandwidthCC.empty());
        region.parseOpcode({ "eq1_bwcc24", "0.5" });
        REQUIRE(region.equalizers[0].bandwidthCC.contains(24));
        REQUIRE(region.equalizers[0].bandwidthCC[24] == 0.5f);
        region.parseOpcode({ "eq1_bw_oncc24", "1.5" });
        REQUIRE(region.equalizers[0].bandwidthCC[24] == 1.5f);
        region.parseOpcode({ "eq3_freqcc15", "10" });
        REQUIRE(region.equalizers[2].frequencyCC.contains(15));
        REQUIRE(region.equalizers[2].frequencyCC[15] == 10.0f);
        region.parseOpcode({ "eq3_freq_oncc15", "20" });
        REQUIRE(region.equalizers[2].frequencyCC[15] == 20.0f);
        region.parseOpcode({ "eq1_type", "hshelf" });
        REQUIRE(region.equalizers[0].type == EqType::kEqHighShelf);
        region.parseOpcode({ "eq2_gaincc123", "2" });
        REQUIRE(region.equalizers[1].gainCC.contains(123));
        REQUIRE(region.equalizers[1].gainCC[123] == 2.0f);
        region.parseOpcode({ "eq2_gain_oncc123", "-2" });
        REQUIRE(region.equalizers[1].gainCC[123] == -2.0f);
    }

    SECTION("EQ parameter values")
    {
        region.parseOpcode({ "eq1_bw", "2" });
        REQUIRE(region.equalizers.size() == 1);
        REQUIRE(region.equalizers[0].bandwidth == 2.0f);
        region.parseOpcode({ "eq1_bw", "5" });
        REQUIRE(region.equalizers[0].bandwidth == 4.0f);
        region.parseOpcode({ "eq1_bw", "0" });
        REQUIRE(region.equalizers[0].bandwidth == 0.001f);
        region.parseOpcode({ "eq1_freq", "300" });
        REQUIRE(region.equalizers[0].frequency == 300.0f);
        region.parseOpcode({ "eq1_freq", "-300" });
        REQUIRE(region.equalizers[0].frequency == 0.0f);
        region.parseOpcode({ "eq1_freq", "35000" });
        REQUIRE(region.equalizers[0].frequency == 30000.0f);
        region.parseOpcode({ "eq1_vel2gain", "4" });
        REQUIRE(region.equalizers[0].vel2gain == 4.0f);
        region.parseOpcode({ "eq1_vel2gain", "250" });
        REQUIRE(region.equalizers[0].vel2gain == 96.0f);
        region.parseOpcode({ "eq1_vel2gain", "-123" });
        REQUIRE(region.equalizers[0].vel2gain == -96.0f);
        region.parseOpcode({ "eq1_vel2freq", "40" });
        REQUIRE(region.equalizers[0].vel2frequency == 40.0f);
        region.parseOpcode({ "eq1_vel2freq", "35000" });
        REQUIRE(region.equalizers[0].vel2frequency == 30000.0f);
        region.parseOpcode({ "eq1_vel2freq", "-35000" });
        REQUIRE(region.equalizers[0].vel2frequency == -30000.0f);
        region.parseOpcode({ "eq1_bwcc15", "2" });
        REQUIRE(region.equalizers[0].bandwidthCC[15] == 2.0f);
        region.parseOpcode({ "eq1_bwcc15", "-5" });
        REQUIRE(region.equalizers[0].bandwidthCC[15] == -4.0f);
        region.parseOpcode({ "eq1_bwcc15", "5" });
        REQUIRE(region.equalizers[0].bandwidthCC[15] == 4.0f);
        region.parseOpcode({ "eq1_gaincc15", "2" });
        REQUIRE(region.equalizers[0].gainCC[15] == 2.0f);
        region.parseOpcode({ "eq1_gaincc15", "-500" });
        REQUIRE(region.equalizers[0].gainCC[15] == -96.0f);
        region.parseOpcode({ "eq1_gaincc15", "500" });
        REQUIRE(region.equalizers[0].gainCC[15] == 96.0f);
        region.parseOpcode({ "eq1_freqcc15", "200" });
        REQUIRE(region.equalizers[0].frequencyCC[15] == 200.0f);
        region.parseOpcode({ "eq1_freqcc15", "-50000" });
        REQUIRE(region.equalizers[0].frequencyCC[15] == -30000.0f);
        region.parseOpcode({ "eq1_freqcc15", "50000" });
        REQUIRE(region.equalizers[0].frequencyCC[15] == 30000.0f);
    }

    SECTION("Effects send")
    {
        REQUIRE(region.gainToEffect.size() == 1);
        REQUIRE(region.gainToEffect[0] == 1.0f);
        region.parseOpcode({ "effect1", "50.4" });
        REQUIRE(region.gainToEffect.size() == 2);
        REQUIRE(region.gainToEffect[1] == 0.504f);
        region.parseOpcode({ "effect3", "100" });
        REQUIRE(region.gainToEffect.size() == 4);
        REQUIRE(region.gainToEffect[2] == 0.0f);
        REQUIRE(region.gainToEffect[3] == 1.0f);
        region.parseOpcode({ "effect3", "150.1" });
        REQUIRE(region.gainToEffect[3] == 1.0f);
        region.parseOpcode({ "effect3", "-50.65" });
        REQUIRE(region.gainToEffect[3] == 0.0f);
    }

    SECTION("Wavetable phase")
    {
        REQUIRE(region.oscillatorPhase == 0.0f);
        region.parseOpcode({ "oscillator_phase", "45" });
        REQUIRE(region.oscillatorPhase == 45.0f);
        region.parseOpcode({ "oscillator_phase", "45.32" });
        REQUIRE(region.oscillatorPhase == 45.32_a);
        region.parseOpcode({ "oscillator_phase", "-1" });
        REQUIRE(region.oscillatorPhase == -1.0f);
        region.parseOpcode({ "oscillator_phase", "361" });
        REQUIRE(region.oscillatorPhase == 360.0f);
    }

    SECTION("Note polyphony")
    {
        REQUIRE(!region.notePolyphony);
        region.parseOpcode({ "note_polyphony", "45" });
        REQUIRE(region.notePolyphony);
        REQUIRE(*region.notePolyphony == 45);
        region.parseOpcode({ "note_polyphony", "-1" });
        REQUIRE(region.notePolyphony);
        REQUIRE(*region.notePolyphony == 0);
    }

    SECTION("Note selfmask")
    {
        REQUIRE(region.selfMask == SfzSelfMask::mask);
        region.parseOpcode({ "note_selfmask", "off" });
        REQUIRE(region.selfMask == SfzSelfMask::dontMask);
        region.parseOpcode({ "note_selfmask", "on" });
        REQUIRE(region.selfMask == SfzSelfMask::mask);
        region.parseOpcode({ "note_selfmask", "off" });
        region.parseOpcode({ "note_selfmask", "garbage" });
        REQUIRE(region.selfMask == SfzSelfMask::dontMask);
    }

    SECTION("amplitude")
    {
        REQUIRE(region.amplitude == 1.0_a);
        region.parseOpcode({ "amplitude", "40" });
        REQUIRE(region.amplitude == 0.4_a);
        region.parseOpcode({ "amplitude", "-40" });
        REQUIRE(region.amplitude == 0_a);
        region.parseOpcode({ "amplitude", "140" });
        REQUIRE(region.amplitude == 1.0_a);
    }

    SECTION("amplitude_cc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Amplitude, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "amplitude_cc1", "40" });
        REQUIRE(view.at(1).value == 40.0_a);
        region.parseOpcode({ "amplitude_oncc2", "30" });
        REQUIRE(view.at(2).value == 30.0_a);
        region.parseOpcode({ "amplitude_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "amplitude_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "amplitude_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "amplitude_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "amplitude_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "amplitude_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "amplitude_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0_a);
        region.parseOpcode({ "amplitude_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 100.0_a);
        region.parseOpcode({ "amplitude_stepcc120", "-2" });
        REQUIRE(view.at(120).step == 0.0f);
    }

    SECTION("volume_oncc/gain_cc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Volume, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "gain_cc1", "40" });
        REQUIRE(view.at(1).value == 40_a);
        region.parseOpcode({ "volume_oncc2", "-76" });
        REQUIRE(view.at(2).value == -76.0_a);
        region.parseOpcode({ "gain_oncc4", "-1" });
        REQUIRE(view.at(4).value == -1.0_a);
        region.parseOpcode({ "volume_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "volume_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "volume_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "volume_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "volume_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "volume_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "volume_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0f);
        region.parseOpcode({ "volume_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 144.0f);
        region.parseOpcode({ "volume_stepcc120", "-2" });
        REQUIRE(view.at(120).step == 0.0f);
    }

    SECTION("tune_cc/pitch_cc")
    {
        const ModKey target = ModKey::createNXYZ(ModId::Pitch, region.getId());
        const RegionCCView view(region, target);
        REQUIRE(view.empty());
        region.parseOpcode({ "pitch_cc1", "40" });
        REQUIRE(view.at(1).value == 40.0);
        region.parseOpcode({ "tune_oncc2", "-76" });
        REQUIRE(view.at(2).value == -76.0);
        region.parseOpcode({ "pitch_oncc4", "-1" });
        REQUIRE(view.at(4).value == -1.0);
        region.parseOpcode({ "tune_curvecc17", "18" });
        REQUIRE(view.at(17).curve == 18);
        region.parseOpcode({ "pitch_curvecc17", "15482" });
        REQUIRE(view.at(17).curve == 255);
        region.parseOpcode({ "tune_curvecc17", "-2" });
        REQUIRE(view.at(17).curve == 0);
        region.parseOpcode({ "pitch_smoothcc14", "85" });
        REQUIRE(view.at(14).smooth == 85);
        region.parseOpcode({ "tune_smoothcc14", "15482" });
        REQUIRE(view.at(14).smooth == 100);
        region.parseOpcode({ "pitch_smoothcc14", "-2" });
        REQUIRE(view.at(14).smooth == 0);
        region.parseOpcode({ "tune_stepcc120", "24" });
        REQUIRE(view.at(120).step == 24.0f);
        region.parseOpcode({ "pitch_stepcc120", "15482" });
        REQUIRE(view.at(120).step == 9600.0f);
        region.parseOpcode({ "tune_stepcc120", "-2" });
        REQUIRE(view.at(120).step == 0.0f);
    }
}

// Specific region bugs
TEST_CASE("[Region] Non-conforming floating point values in integer opcodes")
{
    MidiState midiState;
    Region region { 0, midiState };
    region.parseOpcode({ "offset", "2014.5" });
    REQUIRE(region.offset == 2014);
    region.parseOpcode({ "pitch_keytrack", "-2.1" });
    REQUIRE(region.pitchKeytrack == -2);
}


TEST_CASE("[Region] Release and release key")
{
    MidiState midiState;
    Region region { 0, midiState };
    region.parseOpcode({ "key", "63" });
    region.parseOpcode({ "sample", "*sine" });
    SECTION("Release key without sustain")
    {
        region.parseOpcode({ "trigger", "release_key" });
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
    }
    SECTION("Release key with sustain")
    {
        region.parseOpcode({ "trigger", "release_key" });
        midiState.ccEvent(0, 64, 1.0f);
        REQUIRE( !region.registerCC(64, 1.0f) );
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( !region.registerCC(64, 0.0f) );
    }
    SECTION("Release without sustain")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( region.registerNoteOff(63, 0.5f, 0.0f) );
    }
    SECTION("Release with sustain")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 1.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( !region.registerNoteOff(63, 0.5f, 0.0f) );
    }
    SECTION("Release with sustain")
    {
        region.parseOpcode({ "trigger", "release" });
        midiState.ccEvent(0, 64, 1.0f);
        REQUIRE( !region.registerNoteOn(63, 0.5f, 0.0f) );
        REQUIRE( !region.registerNoteOff(63, 0.5f, 0.0f) );
        midiState.ccEvent(0, 64, 0.0f);
        REQUIRE( region.registerCC(64, 0.0f) );
    }
}
