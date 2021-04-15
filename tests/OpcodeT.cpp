// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Region.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz;

TEST_CASE("[Opcode] Construction")
{
    SECTION("Normal construction")
    {
        Opcode opcode { "sample", "dummy" };
        REQUIRE(opcode.name == "sample");
        REQUIRE(opcode.lettersOnlyHash == hash("sample"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with underscore")
    {
        Opcode opcode { "sample_underscore", "dummy" };
        REQUIRE(opcode.name == "sample_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with ampersand")
    {
        Opcode opcode { "sample&_ampersand", "dummy" };
        REQUIRE(opcode.name == "sample&_ampersand");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_ampersand"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Normal construction with multiple ampersands")
    {
        Opcode opcode { "&sample&_ampersand&", "dummy" };
        REQUIRE(opcode.name == "&sample&_ampersand&");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_ampersand"));
        REQUIRE(opcode.parameters.empty());
        REQUIRE(opcode.value == "dummy");
    }

    SECTION("Parameterized opcode")
    {
        Opcode opcode { "sample123", "dummy" };
        REQUIRE(opcode.name == "sample123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode with ampersand")
    {
        Opcode opcode { "sample&123", "dummy" };
        REQUIRE(opcode.name == "sample&123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode with underscore")
    {
        Opcode opcode { "sample_underscore123", "dummy" };
        REQUIRE(opcode.name == "sample_underscore123");
        REQUIRE(opcode.lettersOnlyHash == hash("sample_underscore&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123 }));
    }

    SECTION("Parameterized opcode within the opcode")
    {
        Opcode opcode { "sample1_underscore", "dummy" };
        REQUIRE(opcode.name == "sample1_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 1 }));
    }

    SECTION("Parameterized opcode within the opcode")
    {
        Opcode opcode { "sample123_underscore", "dummy" };
        REQUIRE(opcode.name == "sample123_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 1);
        REQUIRE(opcode.parameters[0] == 123);
    }

    SECTION("Parameterized opcode within the opcode twice")
    {
        Opcode opcode { "sample123_double44_underscore", "dummy" };
        REQUIRE(opcode.name == "sample123_double44_underscore");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_double&_underscore"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 2);
        REQUIRE(opcode.parameters[0] == 123);
        REQUIRE(opcode.parameters[1] == 44);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123, 44 }));
    }

    SECTION("Parameterized opcode within the opcode twice, with a back parameter")
    {
        Opcode opcode { "sample123_double44_underscore23", "dummy" };
        REQUIRE(opcode.name == "sample123_double44_underscore23");
        REQUIRE(opcode.lettersOnlyHash == hash("sample&_double&_underscore&"));
        REQUIRE(opcode.value == "dummy");
        REQUIRE(opcode.parameters.size() == 3);
        REQUIRE(opcode.parameters == std::vector<uint16_t>({ 123, 44, 23 }));
    }
}

TEST_CASE("[Opcode] Note values")
{
    auto noteValue = readNoteValue("c-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = readNoteValue("C-1");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 0);
    noteValue = readNoteValue("g9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = readNoteValue("G9");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 127);
    noteValue = readNoteValue("c#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue(u8"c♯4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue("C#4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue(u8"C♯4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue("e#4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue(u8"e♯4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue("E#4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue(u8"E♯4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue("db4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue(u8"d♭4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue("Db4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue(u8"D♭4");
    REQUIRE(noteValue);
    REQUIRE(*noteValue == 61);
    noteValue = readNoteValue("fb4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue(u8"f♭4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue("Fb4");
    REQUIRE(!noteValue);
    noteValue = readNoteValue(u8"F♭4");
    REQUIRE(!noteValue);
}

TEST_CASE("[Opcode] Categories")
{
    REQUIRE(Opcode("sample", "").category == kOpcodeNormal);
    REQUIRE(Opcode("amplitude_oncc11", "").category == kOpcodeOnCcN);
    REQUIRE(Opcode("cutoff_cc22", "").category == kOpcodeOnCcN);
    REQUIRE(Opcode("lfo01_pitch_curvecc33", "").category == kOpcodeCurveCcN);
    REQUIRE(Opcode("pan_stepcc44", "").category == kOpcodeStepCcN);
    REQUIRE(Opcode("noise_level_smoothcc55", "").category == kOpcodeSmoothCcN);
}

TEST_CASE("[Opcode] Derived names")
{
    REQUIRE(Opcode("sample", "").getDerivedName(kOpcodeNormal) == "sample");
    REQUIRE(Opcode("cutoff_cc22", "").getDerivedName(kOpcodeNormal) == "cutoff");
    REQUIRE(Opcode("lfo01_pitch_curvecc33", "").getDerivedName(kOpcodeOnCcN) == "lfo01_pitch_oncc33");
    REQUIRE(Opcode("pan_stepcc44", "").getDerivedName(kOpcodeCurveCcN) == "pan_curvecc44");
    REQUIRE(Opcode("noise_level_smoothcc55", "").getDerivedName(kOpcodeStepCcN) == "noise_level_stepcc55");
    REQUIRE(Opcode("sample", "").getDerivedName(kOpcodeSmoothCcN, 66) == "sample_smoothcc66");
}

TEST_CASE("[Opcode] Normalization")
{
    // *_ccN

    REQUIRE(Opcode("foo_cc7", "").cleanUp(kOpcodeScopeRegion).name == "foo_oncc7");
    REQUIRE(Opcode("foo_cc7", "").cleanUp(kOpcodeScopeControl).name == "foo_cc7");

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
        {"bendstep", "bend_step"},
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
        // Cakewalk aliases
        {"cutoff_random", "fil1_random"},
        {"cutoff1_random", "fil1_random"},
        {"cutoff2_random", "fil2_random"},
        {"gain_random", "amp_random"},
        // Internal transformations
        {"ampeg_vel2delay", "ampeg_veltodelay"},
        {"fileg_vel2attack", "fileg_veltoattack"},
        {"pitcheg_vel2decay", "pitcheg_veltodecay"},
        {"ampeg_vel2hold", "ampeg_veltohold"},
        {"fileg_vel2sustain", "fileg_veltosustain"},
        {"pitcheg_vel2release", "pitcheg_veltorelease"},
        {"fileg_vel2depth", "fileg_veltodepth"},
        {"eq21_vel2freq", "eq21_veltofreq"},
        {"eq22_vel2gain", "eq22_veltogain"},
    };

    for (auto pair : regionSpecific) {
        absl::string_view input = pair.first;
        absl::string_view expected = pair.second;
        REQUIRE(Opcode(input, "").cleanUp(kOpcodeScopeRegion).name == expected);
        REQUIRE(Opcode(input, "").cleanUp(kOpcodeScopeGeneric).name == input);
    }

    // <control>

    static const std::pair<absl::string_view, absl::string_view> controlSpecific[] = {
        // ARIA aliases
        {"set_realcc1", "set_hdcc1"},
    };

    for (auto pair : controlSpecific) {
        absl::string_view input = pair.first;
        absl::string_view expected = pair.second;
        REQUIRE(Opcode(input, "").cleanUp(kOpcodeScopeControl).name == expected);
        REQUIRE(Opcode(input, "").cleanUp(kOpcodeScopeGeneric).name == input);
    }

    // case

    REQUIRE(Opcode("SaMpLe", "").cleanUp(kOpcodeScopeRegion).name == "sample");
}

TEST_CASE("[Opcode] opcode read (uint8_t)")
{
    SECTION("Basic")
    {
        Opcode opcode { "", "16" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == 16);
    }

    SECTION("Sign")
    {
        Opcode opcode { "", "+16" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == 16);
    }

    SECTION("Ignore")
    {
        Opcode opcode { "", "110" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == spec.defaultInputValue );
    }

    SECTION("Clamp upper")
    {
        Opcode opcode { "", "110" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), kEnforceUpperBound };
        REQUIRE( opcode.read(spec) == 100 );
    }

    SECTION("Clamp lower")
    {
        Opcode opcode { "", "10" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(20, 100), kEnforceLowerBound };
        REQUIRE( opcode.read(spec) == 20 );
    }

    SECTION("Clamp upper (real)")
    {
        Opcode opcode { "", "101" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(0.0f, 100.5f), kEnforceUpperBound };
        REQUIRE( opcode.read(spec) == 100.5f );
    }

    SECTION("Clamp lower (real)")
    {
        Opcode opcode { "", "19" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(19.5f, 100.0f), kEnforceLowerBound };
        REQUIRE( opcode.read(spec) == 19.5f );
    }

    SECTION("Floating point")
    {
        Opcode opcode { "", "10.5" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == 10 );
    }

    SECTION("Text after")
    {
        Opcode opcode { "", "10garbage" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == 10 );
    }

    SECTION("Text before")
    {
        Opcode opcode { "", "garbage10" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), 0 };
        REQUIRE( !opcode.readOptional(spec) );
        REQUIRE( opcode.read(spec) == 0 );
    }

    SECTION("Can be note")
    {
        Opcode opcode { "", "c4" };
        OpcodeSpec<uint8_t> spec { 0, Range<uint8_t>(0, 100), kCanBeNote };
        REQUIRE( opcode.read(spec) == 60 );
    }
}

TEST_CASE("[Opcode] opcode read (int)")
{
    SECTION("Basic")
    {
        Opcode opcode { "", "16" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), 0 };
        REQUIRE( opcode.read(spec) == 16);
    }

    SECTION("Sign")
    {
        Opcode opcode { "", "+16" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), 0 };
        REQUIRE( opcode.read(spec) == 16);
    }

    SECTION("Sign")
    {
        Opcode opcode { "", "-16" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), 0 };
        REQUIRE( opcode.read(spec) == -16);
    }

    SECTION("Clamp upper")
    {
        Opcode opcode { "", "110" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), kEnforceUpperBound };
        REQUIRE( opcode.read(spec) == 100 );
    }

    SECTION("Clamp lower")
    {
        Opcode opcode { "", "-110" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), kEnforceLowerBound };
        REQUIRE( opcode.read(spec) == -100 );
    }

    SECTION("Floating point")
    {
        Opcode opcode { "", "10.5" };
        OpcodeSpec<int> spec { 0, Range<int>(-100, 100), 0 };
        REQUIRE( opcode.read(spec) == 10 );
    }

    SECTION("Text after")
    {
        Opcode opcode { "", "10garbage" };
        OpcodeSpec<int> spec { 0, Range<int>(0, 100), 0 };
        REQUIRE( opcode.read(spec) == 10 );
    }

    SECTION("Text before")
    {
        Opcode opcode { "", "garbage10" };
        OpcodeSpec<int> spec { 0, Range<int>(20, 100), 0 };
        REQUIRE( !opcode.readOptional(spec) );
        REQUIRE( opcode.read(spec) == 0 );
    }

    SECTION("Can be note")
    {
        Opcode opcode { "", "c4" };
        OpcodeSpec<int> spec { 0, Range<int>(20, 100), kCanBeNote };
        REQUIRE( opcode.read(spec) == 60 );
    }
}


TEST_CASE("[Opcode] opcode read (float)")
{
    SECTION("Basic")
    {
        Opcode opcode { "", "16.4" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), 0 };
        REQUIRE( opcode.read(spec) == 16.4_a);
    }

    SECTION("Plus sign")
    {
        Opcode opcode { "", "+16.4" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), 0 };
        REQUIRE( opcode.read(spec) == 16.4_a);
    }

    SECTION("Minus sign")
    {
        Opcode opcode { "", "-16.4" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), 0 };
        REQUIRE( opcode.read(spec) == -16.4_a);
    }

    SECTION("Ignore")
    {
        Opcode opcode { "", "110" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), 0 };
        REQUIRE( opcode.read(spec) == spec.defaultInputValue );
    }

    SECTION("Clamp upper")
    {
        Opcode opcode { "", "110" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), kEnforceUpperBound };
        REQUIRE( opcode.read(spec) == 100.0f );
    }

    SECTION("Clamp lower")
    {
        Opcode opcode { "", "-110" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(-100.0f, 100.0f), kEnforceLowerBound };
        REQUIRE( opcode.read(spec) == -100.0f );
    }

    SECTION("Text after")
    {
        Opcode opcode { "", "10.5garbage" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(0.0f, 100.0f), kEnforceLowerBound };
        REQUIRE( opcode.read(spec) == 10.5f );
    }

    SECTION("Text before")
    {
        Opcode opcode { "", "garbage10" };
        OpcodeSpec<float> spec { 0.0f, Range<float>(0.0f, 100.0f), 0 };
        REQUIRE( !opcode.readOptional(spec) );
        REQUIRE( opcode.read(spec) == 0.0f );
    }
}

TEST_CASE("[Opcode] readBooleanFromOpcode")
{
    REQUIRE(readBoolean({"1"}) == true);
    REQUIRE(readBoolean({"0"}) == false);
    REQUIRE(readBoolean({"777"}) == true);
    REQUIRE(readBoolean({"on"}) == true);
    REQUIRE(readBoolean({"off"}) == false);
    REQUIRE(readBoolean({"On"}) == true);
    REQUIRE(readBoolean({"oFf"}) == false);
}
