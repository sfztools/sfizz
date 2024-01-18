// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SynthDiscussion.h"
#include "SfzHelpers.h"
#include "catch2/catch.hpp"

using namespace Catch::literals;
using namespace sfz;
using namespace sfz::literals;
using namespace std::literals;

using OSC = OSCValueLess;

TEST_CASE("Set values", "[parsing][OSC]")
{
    SynthDiscussion d;

    SECTION("Pitch keycenter") {
        d.load(R"( <region> sample=*sine pitch_keycenter=48 )");
        REQUIRE( d.read<int32_t>("/region0/pitch_keycenter") == 48);
        REQUIRE( d.sendAndRead("/region0/pitch_keycenter", 60) == 60);
    }

    SECTION("LFO Wave") {
        d.load(R"( <region> sample=*sine lfo1_wave=5 lfo1_wave2=4 )");
        REQUIRE( d.read<int32_t>("/region0/lfo0/wave") == 5);
        REQUIRE( d.read<int32_t>("/region0/lfo0/wave1") == 4);
        REQUIRE( d.sendAndRead("/region0/lfo0/wave", 3) == 3);
        REQUIRE( d.sendAndRead("/region0/lfo0/wave1", 2) == 2);
    }

    SECTION("Loop mode") {
        d.load(R"( <region> sample=looped_flute.wav )");
        REQUIRE( d.read<std::string>("/region0/loop_mode") == "loop_continuous");
        REQUIRE( d.sendAndRead<std::string>("/region0/loop_mode", "one_shot") == "one_shot");
    }

    SECTION("Sample quality") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<int32_t>("/sample_quality") == 2);
        REQUIRE( d.read<int32_t>("/oscillator_quality") == 1);
        REQUIRE( d.read<int32_t>("/freewheeling_sample_quality") == 10);
        REQUIRE( d.read<int32_t>("/freewheeling_oscillator_quality") == 3);
        REQUIRE( d.sendAndRead("/sample_quality", 3) == 3);
        REQUIRE( d.sendAndRead("/oscillator_quality", 2) == 2);
        REQUIRE( d.sendAndRead("/freewheeling_sample_quality", 6) == 6);
        REQUIRE( d.sendAndRead("/freewheeling_oscillator_quality", 2) == 2);
    }

    SECTION("Sustain cancels release") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<OSC>("/sustain_cancels_release") == OSC::False );
        d.send("/sustain_cancels_release", true);
        REQUIRE( d.read<OSC>("/sustain_cancels_release") == OSC::True );
        d.send("/sustain_cancels_release", false);
        REQUIRE( d.read<OSC>("/sustain_cancels_release") == OSC::False );
        d.send("/sustain_cancels_release", "on"s);
        REQUIRE( d.read<OSC>("/sustain_cancels_release") == OSC::True );
    }

    SECTION("Delay") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/delay", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/delay_random", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/delay_cc1", 10.0f) == 10.0f);
    }

    SECTION("Offset") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int64_t>("/region0/offset", 10) == 10);
        REQUIRE( d.sendAndRead<int64_t>("/region0/offset_random", 10) == 10);
        REQUIRE( d.sendAndRead<int64_t>("/region0/offset_cc1", 10) == 10);
    }

    SECTION("End") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int64_t>("/region0/end", 10) == 10);
        REQUIRE( d.sendAndRead<int64_t>("/region0/end_cc1", 10) == 10);
    }

    SECTION("Count") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/count", 3) == 3);
    }

    SECTION("Loop range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<int64_t> v {13, 2000};
        REQUIRE( d.sendAndReadAll<int64_t>("/region0/loop_range", v) == v);
        REQUIRE( d.sendAndRead<int64_t>("/region0/loop_start_cc1", 10) == 10);
        REQUIRE( d.sendAndRead<int64_t>("/region0/loop_end_cc1", 1000) == 1000);
    }

    SECTION("Loop count") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/loop_count", 3) == 3);
        d.send("/region0/loop_count", nullptr);
        REQUIRE( d.read<OSC>("/region0/loop_count") == OSC::None);
    }

    SECTION("Output") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/output", 3) == 3);
    }

    SECTION("Off by") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<OSC>("/region0/off_by") == OSC::None);
        REQUIRE( d.sendAndRead<int64_t>("/region0/off_by", 2) == 2);
        d.send("/region0/off_by", nullptr);
        REQUIRE( d.read<OSC>("/region0/off_by") == OSC::None);
    }

    SECTION("Off mode") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<std::string>("/region0/off_mode", "time") =="time");
        REQUIRE( d.sendAndRead<std::string>("/region0/off_mode", "fast") =="fast");
    }

    SECTION("Key range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<int32_t> v {5, 67};
        REQUIRE( d.sendAndReadAll<int32_t>("/region0/key_range", v) == v);
    }

    SECTION("Off time") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<float>("/region0/off_time", 0.1f) == 0.1f);
    }

    SECTION("Velocity range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/vel_range", v) == v);
    }

    SECTION("Bend range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_bend, 67_bend};
        REQUIRE( d.sendAndReadAll<float>("/region0/bend_range", v) == v);
    }

    SECTION("Program range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<int32_t> v {2, 10};
        REQUIRE( d.sendAndReadAll<int32_t>("/region0/program_range", v) == v);
    }

    SECTION("CC range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/cc_range2", v) == v);
    }

    SECTION("Last keyswitch") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/sw_last", 24) == 24);
        std::vector<int32_t> v {10, 15};
        REQUIRE( d.sendAndReadAll<int32_t>("/region0/sw_last", v) == v);
    }

    SECTION("Keyswitch label") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<std::string>("/region0/sw_label", "hello") == "hello");
    }

    SECTION("Keyswitch up") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/sw_up", 12) == 12);
        d.send("/region0/sw_up", "c4"s);
        REQUIRE( d.read<int32_t>("/region0/sw_up") == 60);
    }

    SECTION("Keyswitch down") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/sw_down", 12) == 12);
        d.send("/region0/sw_down", "c4"s);
        REQUIRE( d.read<int32_t>("/region0/sw_down") == 60);
    }

    SECTION("Keyswitch down") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/sw_previous", 12) == 12);
        d.send("/region0/sw_previous", "c4"s);
        REQUIRE( d.read<int32_t>("/region0/sw_previous") == 60);
    }

    SECTION("Velocity override") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<std::string>("/region0/sw_vel", "previous") == "previous");
    }

    SECTION("Channel aftertouch range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/chanaft_range", v) == v);
    }

    SECTION("Poly aftertouch range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/polyaft_range", v) == v);
    }

    SECTION("BPM range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5, 67};
        REQUIRE( d.sendAndReadAll<float>("/region0/bpm_range", v) == v);
    }

    SECTION("Rand range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/rand_range", v) == v);
    }

    SECTION("Sequences") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/seq_length", 2) == 2);
        REQUIRE( d.sendAndRead<int32_t>("/region0/seq_position", 2) == 2);
    }

    SECTION("Trigger") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<std::string>("/region0/trigger", "release") == "release");
    }

    SECTION("Start CC range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/start_cc_range2", v) == v);
    }

    SECTION("Volume etc") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/volume", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/pan", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/width", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/position", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/amplitude", 10.0f) == 10.0f);
    }

    SECTION("Amp key something") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/amp_keycenter", 48) == 48);
        REQUIRE( d.sendAndRead("/region0/amp_keytrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/amp_veltrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/amp_veltrack_cc3", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/amp_veltrack_curvecc3", 2) == 2);
    }

    SECTION("Amp random") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/amp_random", 10.0f) == 10.0f);
    }

    SECTION("Crossfade key range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<int32_t> v {5, 67};
        REQUIRE( d.sendAndReadAll<int32_t>("/region0/xfin_key_range", v) == v);
        REQUIRE( d.sendAndReadAll<int32_t>("/region0/xfout_key_range", v) == v);
    }

    SECTION("Other crossfade range") {
        d.load(R"( <region> sample=kick.wav )");
        std::vector<float> v {5_norm, 67_norm};
        REQUIRE( d.sendAndReadAll<float>("/region0/xfin_vel_range", v) == v);
        REQUIRE( d.sendAndReadAll<float>("/region0/xfout_vel_range", v) == v);
        REQUIRE( d.sendAndReadAll<float>("/region0/xfin_cc_range3", v) == v);
        REQUIRE( d.sendAndReadAll<float>("/region0/xfout_cc_range3", v) == v);
    }

    SECTION("Crossfade curves") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<std::string>("/region0/xf_keycurve", "power") == "power");
        REQUIRE( d.sendAndRead<std::string>("/region0/xf_velcurve", "power") == "power");
        REQUIRE( d.sendAndRead<std::string>("/region0/xf_cccurve", "power") == "power");
    }

    SECTION("Global amps and volumes curves") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<float>("/region0/global_volume", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/master_volume", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/group_volume", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/global_amplitude", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/master_amplitude", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/group_amplitude", 10.0f) == 10.0f);
    }

    SECTION("Pitch and transpose") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/pitch", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/transpose", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/pitch_random", 10.0f) == 10.0f);
    }

    SECTION("Pitch key something") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/pitch_keycenter", 48) == 48);
        REQUIRE( d.sendAndRead("/region0/pitch_keytrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/pitch_veltrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/pitch_veltrack_cc3", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/pitch_veltrack_curvecc3", 2) == 2);
    }

    SECTION("Bends") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/bend_up", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/bend_down", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/bend_step", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead("/region0/bend_smooth", 10) == 10);
    }

    SECTION("Ampeg") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/ampeg_attack", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_delay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_decay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_hold", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_release", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_start", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_sustain", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_depth", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_attack_cc1", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_decay_cc2", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_delay_cc3", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_hold_cc4", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_release_cc5", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_sustain_cc6", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_start_cc7", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_attack_curvecc1", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_decay_curvecc2", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_delay_curvecc3", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_hold_curvecc4", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_release_curvecc5", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_sustain_curvecc6", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_start_curvecc7", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2attack", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2delay", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2decay", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2hold", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2release", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2sustain", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/ampeg_vel2depth", 1.0f) == 1.0f);
        REQUIRE( d.read<OSC>("/region0/ampeg_dynamic") == OSC::False);
        d.send("/region0/ampeg_dynamic", true);
        REQUIRE( d.read<OSC>("/region0/ampeg_dynamic") == OSC::True);
        d.send("/region0/ampeg_dynamic", "off"s);
        REQUIRE( d.read<OSC>("/region0/ampeg_dynamic") == OSC::False);
    }

    SECTION("Fileg") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/fileg_attack", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_delay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_decay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_hold", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_release", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_start", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_sustain", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_depth", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_attack_cc1", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_decay_cc2", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_delay_cc3", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_hold_cc4", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_release_cc5", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_sustain_cc6", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_start_cc7", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/fileg_attack_curvecc1", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_decay_curvecc2", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_delay_curvecc3", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_hold_curvecc4", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_release_curvecc5", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_sustain_curvecc6", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/fileg_start_curvecc7", 2) == 2);
        REQUIRE( d.read<OSC>("/region0/fileg_dynamic") == OSC::False);
        d.send("/region0/fileg_dynamic", true);
        REQUIRE( d.read<OSC>("/region0/fileg_dynamic") == OSC::True);
        d.send("/region0/fileg_dynamic", "off"s);
        REQUIRE( d.read<OSC>("/region0/fileg_dynamic") == OSC::False);
    }

    SECTION("PitchEG") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead("/region0/pitcheg_attack", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_delay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_decay", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_hold", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_release", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_start", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_sustain", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_depth", 2.0f) == 2.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_attack_cc1", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_decay_cc2", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_delay_cc3", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_hold_cc4", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_release_cc5", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_sustain_cc6", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_start_cc7", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead("/region0/pitcheg_attack_curvecc1", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_decay_curvecc2", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_delay_curvecc3", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_hold_curvecc4", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_release_curvecc5", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_sustain_curvecc6", 2) == 2);
        REQUIRE( d.sendAndRead("/region0/pitcheg_start_curvecc7", 2) == 2);
        REQUIRE( d.read<OSC>("/region0/pitcheg_dynamic") == OSC::False);
        d.send("/region0/pitcheg_dynamic", true);
        REQUIRE( d.read<OSC>("/region0/pitcheg_dynamic") == OSC::True);
        d.send("/region0/pitcheg_dynamic", "off"s);
        REQUIRE( d.read<OSC>("/region0/pitcheg_dynamic") == OSC::False);
    }

    SECTION("Note polyphony") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<int32_t>("/region0/note_polyphony", 3) == 3);
    }

    SECTION("RT dead") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<OSC>("/region0/rt_dead") == OSC::False );
        d.send("/region0/rt_dead", true);
        REQUIRE( d.read<OSC>("/region0/rt_dead") == OSC::True );
        d.send("/region0/rt_dead", false);
        REQUIRE( d.read<OSC>("/region0/rt_dead") == OSC::False );
        d.send("/region0/rt_dead", "on"s);
        REQUIRE( d.read<OSC>("/region0/rt_dead") == OSC::True );
    }

    SECTION("Sustain/sostenuto") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<OSC>("/region0/sustain_sw") == OSC::True );
        d.send("/region0/sustain_sw", false);
        REQUIRE( d.read<OSC>("/region0/sustain_sw") == OSC::False );
        d.send("/region0/sustain_sw", true);
        REQUIRE( d.read<OSC>("/region0/sustain_sw") == OSC::True );
        d.send("/region0/sustain_sw", "off"s);
        REQUIRE( d.read<OSC>("/region0/sustain_sw") == OSC::False );
        REQUIRE( d.read<OSC>("/region0/sostenuto_sw") == OSC::True );
        d.send("/region0/sostenuto_sw", false);
        REQUIRE( d.read<OSC>("/region0/sostenuto_sw") == OSC::False );
        d.send("/region0/sostenuto_sw", true);
        REQUIRE( d.read<OSC>("/region0/sostenuto_sw") == OSC::True );
        d.send("/region0/sostenuto_sw", "off"s);
        REQUIRE( d.read<OSC>("/region0/sostenuto_sw") == OSC::False );
        REQUIRE( d.sendAndRead<int32_t>("/region0/sustain_cc", 23) == 23);
        REQUIRE( d.sendAndRead<int32_t>("/region0/sostenuto_cc", 23) == 23);
        REQUIRE( d.sendAndRead<float>("/region0/sustain_lo", 0.1f) == 0.1f);
        REQUIRE( d.sendAndRead<float>("/region0/sostenuto_lo", 0.1f) == 0.1f);
    }

    SECTION("Note selfmask") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.read<OSC>("/region0/note_selfmask") == OSC::True );
        d.send("/region0/note_selfmask", "off"s);
        REQUIRE( d.read<OSC>("/region0/note_selfmask") == OSC::False );
        d.send("/region0/note_selfmask", "mask"s);
        REQUIRE( d.read<OSC>("/region0/note_selfmask") == OSC::True );
    }

    SECTION("Oscillator stuff") {
        d.load(R"( <region> sample=kick.wav )");
        REQUIRE( d.sendAndRead<float>("/region0/oscillator_phase", 0.1f) == 0.1f);
        REQUIRE( d.sendAndRead<int32_t>("/region0/oscillator_quality", 2) == 2);
        REQUIRE( d.sendAndRead<int32_t>("/region0/oscillator_mode", 1) == 1);
        REQUIRE( d.sendAndRead<int32_t>("/region0/oscillator_multi", 5) == 5);
        REQUIRE( d.sendAndRead<float>("/region0/oscillator_detune", 0.2f) == 0.2f);
        REQUIRE( d.sendAndRead<float>("/region0/oscillator_mod_depth", 0.2f) == 0.2f);
    }

    SECTION("Effect") {
        d.load(R"( <region> sample=kick.wav effect1=10)");
        REQUIRE( d.sendAndRead<float>("/region0/effect1", 1.0f) == 1.0f);
    }

    SECTION("Filters") {
        d.load(R"( <region> sample=kick.wav)");
        REQUIRE(d.read<int32_t>("/region0/add_filter") == 0);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/cutoff", 100.0f) == 100.0f);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/resonance", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/gain", 4.0f) == 4.0f);
        REQUIRE( d.sendAndRead<int32_t>("/region0/filter0/keycenter", 42) == 42);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/keytrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/veltrack", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/filter0/veltrack_cc1", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<int32_t>("/region0/filter0/veltrack_curvecc2", 3) == 3);
        REQUIRE( d.sendAndRead<std::string>("/region0/filter0/type", "lpf_2p") == "lpf_2p");
    }

    SECTION("EQs") {
        d.load(R"( <region> sample=kick.wav)");
        REQUIRE(d.read<int32_t>("/region0/add_eq") == 0);
        REQUIRE( d.sendAndRead<float>("/region0/eq0/gain", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/eq0/bandwidth", 100.0f) == 100.0f);
        REQUIRE( d.sendAndRead<float>("/region0/eq0/frequency", 500.0f) == 500.0f);
        REQUIRE( d.sendAndRead<float>("/region0/eq0/vel2freq", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<float>("/region0/eq0/vel2gain", 10.0f) == 10.0f);
        REQUIRE( d.sendAndRead<std::string>("/region0/eq0/type", "hshelf") == "hshelf");
    }

    SECTION("EGs") {
        d.load(R"( <region> sample=kick.wav)");
        REQUIRE(d.read<int32_t>("/region0/add_eg") == 0);
        REQUIRE(d.read<int32_t>("/region0/eg0/add_point") == 0);
        REQUIRE( d.sendAndRead<float>("/region0/eg0/point0/time", 1.0f) == 1.0f);
        REQUIRE( d.sendAndRead<float>("/region0/eg0/point0/level", 0.5f) == 0.5f);
    }
}
