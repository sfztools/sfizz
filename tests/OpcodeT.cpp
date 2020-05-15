// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;

TEST_CASE("[Opcode] Construction")
{
    SECTION("Normal construction")
    {
        sfz::Opcode opcode { "sample", "dummy" };
        REQUIRE(opcode.opcode == "sample");
        REQUIRE(opcode.lettersOnlyHash == hash("sample"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with underscore")
    {
        sfz::Opcode opcode { "sample_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with ampersand")
    {
        sfz::Opcode opcode { "sample&_ampersand", "dummy" };
        REQUIRE(opcode.opcode == "sample&_ampersand");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_ampersand"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with multiple ampersands")
    {
        sfz::Opcode opcode { "&sample&_ampersand&", "dummy" };
        REQUIRE(opcode.opcode == "&sample&_ampersand&");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_ampersand"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Parameterized opcode")
    {
        sfz::Opcode opcode { "sample123", "dummy" };
        REQUIRE(opcode.opcode == "sample123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode with ampersand")
    {
        sfz::Opcode opcode { "sample&123", "dummy" };
        REQUIRE(opcode.opcode == "sample&123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode with underscore")
    {
        sfz::Opcode opcode { "sample_underscore123", "dummy" };
        REQUIRE(opcode.opcode == "sample_underscore123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode within the opcode")
    {
        sfz::Opcode opcode { "sample1_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample1_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 1 }));
    }

    SECTION("Parameterized opcode within the opcode")
    {
        sfz::Opcode opcode { "sample123_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample123_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters[0] == 123);
    }

    SECTION("Parameterized opcode within the opcode twice")
    {
        sfz::Opcode opcode { "sample123_double44_underscore", "dummy" };
        REQUIRE(opcode.opcode == "sample123_double44_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_double&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 2);
        REQUIRE(opcode.parameters[0] == 123);
        REQUIRE(opcode.parameters[1] == 44);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123, 44 }));
    }

    SECTION("Parameterized opcode within the opcode twice, with a back parameter")
    {
        sfz::Opcode opcode { "sample123_double44_underscore23", "dummy" };
        REQUIRE(opcode.opcode == "sample123_double44_underscore23");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_double&_underscore&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 3);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123, 44, 23 }));
    }
}

TEST_CASE("[Opcode] Note values")
{
    auto noteValue = sfz::readNoteValue("c-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = sfz::readNoteValue("C-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = sfz::readNoteValue("g9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = sfz::readNoteValue("G9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = sfz::readNoteValue("c#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = sfz::readNoteValue("C#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
}

TEST_CASE("[Opcode] Categories")
{
    REQUIRE(sfz::Opcode("sample", "").category == sfz::kOpcodeNormal);
    REQUIRE(sfz::Opcode("amplitude_oncc11", "").category == sfz::kOpcodeOnCcN);
    REQUIRE(sfz::Opcode("cutoff_cc22", "").category == sfz::kOpcodeOnCcN);
    REQUIRE(sfz::Opcode("lfo01_pitch_curvecc33", "").category == sfz::kOpcodeCurveCcN);
    REQUIRE(sfz::Opcode("pan_stepcc44", "").category == sfz::kOpcodeStepCcN);
    REQUIRE(sfz::Opcode("noise_level_smoothcc55", "").category == sfz::kOpcodeSmoothCcN);
}

TEST_CASE("[Opcode] Derived names")
{
    REQUIRE(sfz::Opcode("sample", "").getDerivedName(sfz::kOpcodeNormal) == "sample");
    REQUIRE(sfz::Opcode("cutoff_cc22", "").getDerivedName(sfz::kOpcodeNormal) == "cutoff");
    REQUIRE(sfz::Opcode("lfo01_pitch_curvecc33", "").getDerivedName(sfz::kOpcodeOnCcN) == "lfo01_pitch_oncc33");
    REQUIRE(sfz::Opcode("pan_stepcc44", "").getDerivedName(sfz::kOpcodeCurveCcN) == "pan_curvecc44");
    REQUIRE(sfz::Opcode("noise_level_smoothcc55", "").getDerivedName(sfz::kOpcodeStepCcN) == "noise_level_stepcc55");
    REQUIRE(sfz::Opcode("sample", "").getDerivedName(sfz::kOpcodeSmoothCcN, 66) == "sample_smoothcc66");
}

TEST_CASE("[Opcode] Normalization")
{
    // *_ccN

    REQUIRE(sfz::Opcode("foo_cc7", "").cleanUp(sfz::kOpcodeScopeRegion).opcode == "foo_oncc7");
    REQUIRE(sfz::Opcode("foo_cc7", "").cleanUp(sfz::kOpcodeScopeControl).opcode == "foo_cc7");

    // <region>

    static const std::pair<absl::string_view, absl::string_view> regionSpecific[] = {
        // LFO SFZv1
        {"amplfo_depthcc1", "amplfo_depth_oncc1"},
        {"fillfo_freqcc2", "fillfo_freq_oncc2"},
        {"pitchlfo_fadecc3", "pitchlfo_fade_oncc3"},
        // EG SFZv1
        {"ampeg_delaycc4", "ampeg_delay_oncc4"},
        {"fileg_startcc5", "fileg_start_oncc5"},
        {"pitcheg_attackcc6", "pitcheg_attack_oncc6"},
        {"ampeg_holdcc7", "ampeg_hold_oncc7"},
        {"fileg_decaycc8", "fileg_decay_oncc8"},
        {"pitcheg_sustaincc9", "pitcheg_sustain_oncc9"},
        {"ampeg_releasecc10", "ampeg_release_oncc10"},
        // EQ SFZv1
        {"eq11_bwcc12", "eq11_bw_oncc12"},
        {"eq13_freqcc14", "eq13_freq_oncc14"},
        {"eq15_gaincc16", "eq15_gain_oncc16"},
        // LFO SFZv2
        {"lfo17_wave", "lfo17_wave1"},
        {"lfo18_offset", "lfo18_offset1"},
        {"lfo19_ratio", "lfo19_ratio1"},
        {"lfo20_scale", "lfo20_scale1"},
        // LinuxSampler aliases
        {"loopmode", "loop_mode"},
        {"loopstart", "loop_start"},
        {"loopend", "loop_end"},
        {"offby", "off_by"},
        {"offmode", "off_mode"},
        {"bendup", "bend_up"},
        {"benddown", "bend_down"},
        {"filtype", "fil1_type"},
        {"fil21type", "fil21_type"},
        // ARIA aliases
        {"polyphony_group", "group"},
        {"gain", "volume"},
        {"gain_foobar", "volume_foobar"},
        {"tune", "pitch"},
        {"tune_foobar", "pitch_foobar"},
        {"lorealcc24", "lohdcc24"},
        {"hirealcc25", "hihdcc25"},
        {"on_lohdcc26", "start_lohdcc26"},
        {"on_hihdcc27", "start_hihdcc27"},
        // SFZv2 aliases
        {"on_hicc22", "start_hicc22"},
        {"on_locc23", "start_locc23"},
        // Filter SFZv1
        {"fil_foobar", "fil1_foobar"},
        {"cutoff", "cutoff1"},
        {"cutoff_foobar", "cutoff1_foobar"},
        {"resonance", "resonance1"},
        {"resonance_foobar", "resonance1_foobar"},
    };

    for (auto pair : regionSpecific) {
        absl::string_view input = pair.first;
        absl::string_view expected = pair.second;
        REQUIRE(sfz::Opcode(input, "").cleanUp(sfz::kOpcodeScopeRegion).opcode == expected);
        REQUIRE(sfz::Opcode(input, "").cleanUp(sfz::kOpcodeScopeGeneric).opcode == input);
    }

    // <control>

    static const std::pair<absl::string_view, absl::string_view> controlSpecific[] = {
        // ARIA aliases
        {"set_realcc1", "set_hdcc1"},
    };

    for (auto pair : controlSpecific) {
        absl::string_view input = pair.first;
        absl::string_view expected = pair.second;
        REQUIRE(sfz::Opcode(input, "").cleanUp(sfz::kOpcodeScopeControl).opcode == expected);
        REQUIRE(sfz::Opcode(input, "").cleanUp(sfz::kOpcodeScopeGeneric).opcode == input);
    }

    // case

    REQUIRE(sfz::Opcode("SaMpLe", "").cleanUp(sfz::kOpcodeScopeRegion).opcode == "sample");
}
