// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @brief MPE (MIDI Polyphonic Expression) regression tests.
 *
 * Covers per-channel state isolation in MidiState, channel-aware writes
 * via the Synth::*MPE public API, propagation of channel into TriggerEvent
 * and Voice, and same-channel-preference behavior of voice stealing when
 * MPE is enabled. With MPE disabled (the default) all paths reduce to
 * master-channel reads and the existing single-channel tests continue to
 * pass unchanged.
 */

#include "sfizz/MidiState.h"
#include "sfizz/Synth.h"
#include "sfizz/Voice.h"
#include "sfizz/SfzHelpers.h"
#include "catch2/catch.hpp"
using namespace Catch::literals;
using namespace sfz::literals;

// =============================================================================
// MidiState: per-channel writes/reads in isolation
// =============================================================================

TEST_CASE("[MPE] MidiState pitch bend is per-channel")
{
    sfz::MidiState state;
    state.pitchBendEvent(0, /*channel=*/1, 0.3f);
    state.pitchBendEvent(0, /*channel=*/2, -0.4f);
    state.pitchBendEvent(0, /*channel=*/3, 0.7f);

    REQUIRE(state.getPitchBend(1) == 0.3f);
    REQUIRE(state.getPitchBend(2) == -0.4f);
    REQUIRE(state.getPitchBend(3) == 0.7f);
    // Master channel was not written, so it should still read 0.
    REQUIRE(state.getPitchBend(0) == 0.0f);
    // The single-arg overload defaults to master.
    REQUIRE(state.getPitchBend() == 0.0f);
}

TEST_CASE("[MPE] MidiState CC values are per-channel")
{
    sfz::MidiState state;
    state.ccEvent(0, /*channel=*/1, 74, 0.3f);
    state.ccEvent(0, /*channel=*/2, 74, 0.7f);
    state.ccEvent(0, /*channel=*/2, 11, 0.5f);

    REQUIRE(state.getCCValue(1, 74) == 0.3f);
    REQUIRE(state.getCCValue(2, 74) == 0.7f);
    REQUIRE(state.getCCValue(2, 11) == 0.5f);
    // Channel 1's CC11 was never written.
    REQUIRE(state.getCCValue(1, 11) == 0.0f);
    // Master untouched.
    REQUIRE(state.getCCValue(0, 74) == 0.0f);
    REQUIRE(state.getCCValue(74) == 0.0f);
}

TEST_CASE("[MPE] MidiState channel aftertouch is per-channel")
{
    sfz::MidiState state;
    state.channelAftertouchEvent(0, /*channel=*/1, 0.3f);
    state.channelAftertouchEvent(0, /*channel=*/2, 0.7f);

    REQUIRE(state.getChannelAftertouch(1) == 0.3f);
    REQUIRE(state.getChannelAftertouch(2) == 0.7f);
    REQUIRE(state.getChannelAftertouch(0) == 0.0f);
    REQUIRE(state.getChannelAftertouch() == 0.0f);
}

TEST_CASE("[MPE] MidiState polyphonic aftertouch is per-channel")
{
    sfz::MidiState state;
    state.polyAftertouchEvent(0, /*channel=*/1, /*note=*/60, 0.3f);
    state.polyAftertouchEvent(0, /*channel=*/2, /*note=*/60, 0.7f);
    state.polyAftertouchEvent(0, /*channel=*/2, /*note=*/64, 0.5f);

    REQUIRE(state.getPolyAftertouch(1, 60) == 0.3f);
    REQUIRE(state.getPolyAftertouch(2, 60) == 0.7f);
    REQUIRE(state.getPolyAftertouch(2, 64) == 0.5f);
    REQUIRE(state.getPolyAftertouch(1, 64) == 0.0f);
    REQUIRE(state.getPolyAftertouch(0, 60) == 0.0f);
    REQUIRE(state.getPolyAftertouch(60) == 0.0f);
}

TEST_CASE("[MPE] MidiState single-arg overloads forward to master")
{
    sfz::MidiState state;
    // Write via master via the single-arg overloads, read both ways.
    state.pitchBendEvent(0, 0.5f);
    state.ccEvent(0, 64, 0.6f);
    state.channelAftertouchEvent(0, 0.7f);
    state.polyAftertouchEvent(0, 60, 0.8f);

    REQUIRE(state.getPitchBend() == 0.5f);
    REQUIRE(state.getPitchBend(0) == 0.5f);
    REQUIRE(state.getCCValue(64) == 0.6f);
    REQUIRE(state.getCCValue(0, 64) == 0.6f);
    REQUIRE(state.getChannelAftertouch() == 0.7f);
    REQUIRE(state.getChannelAftertouch(0) == 0.7f);
    REQUIRE(state.getPolyAftertouch(60) == 0.8f);
    REQUIRE(state.getPolyAftertouch(0, 60) == 0.8f);
}

TEST_CASE("[MPE] MidiState out-of-range channels are no-ops on write and 0 on read")
{
    sfz::MidiState state;
    // These should be silently dropped, not crash, not corrupt master.
    state.pitchBendEvent(0, /*channel=*/-1, 0.5f);
    state.pitchBendEvent(0, /*channel=*/16, 0.5f);
    state.pitchBendEvent(0, /*channel=*/99, 0.5f);
    state.ccEvent(0, /*channel=*/-1, 64, 0.5f);
    state.ccEvent(0, /*channel=*/99, 64, 0.5f);
    state.channelAftertouchEvent(0, /*channel=*/-1, 0.5f);
    state.polyAftertouchEvent(0, /*channel=*/-1, 60, 0.5f);

    // Master remains untouched.
    REQUIRE(state.getPitchBend(0) == 0.0f);
    REQUIRE(state.getCCValue(0, 64) == 0.0f);
    REQUIRE(state.getChannelAftertouch(0) == 0.0f);
    REQUIRE(state.getPolyAftertouch(0, 60) == 0.0f);

    // Out-of-range reads return 0.
    REQUIRE(state.getPitchBend(-1) == 0.0f);
    REQUIRE(state.getPitchBend(16) == 0.0f);
    REQUIRE(state.getPitchBend(99) == 0.0f);
}

// =============================================================================
// Synth: *MPE public API routes events to the right channel slot
// =============================================================================

TEST_CASE("[MPE] Synth::pitchWheelMPE lands in the per-channel pitch slot")
{
    sfz::Synth synth;
    synth.pitchWheelMPE(0, 1, 4096);
    synth.pitchWheelMPE(0, 2, -4096);

    // pitchWheel takes a 14-bit centered-zero value (-8192..+8191 effective)
    // normalized to roughly -1..+1. The exact divisor differs by sign in
    // sfizz's normalizeBend, so the values are approximate; a margin of
    // 0.001 is plenty to catch the per-channel routing while tolerating
    // the asymmetry.
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPitchBend(1) == Approx(0.5).margin(0.001));
    REQUIRE(mid.getPitchBend(2) == Approx(-0.5).margin(0.001));
    REQUIRE(mid.getPitchBend(0) == Approx(0.0).margin(0.001));
}

TEST_CASE("[MPE] Synth::ccMPE lands in the per-channel CC slot")
{
    sfz::Synth synth;
    synth.ccMPE(0, 1, 74, 64);
    synth.ccMPE(0, 2, 74, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(1, 74) == 64_norm);
    REQUIRE(mid.getCCValue(2, 74) == 127_norm);
    REQUIRE(mid.getCCValue(0, 74) == 0.0_a);
}

TEST_CASE("[MPE] Synth::channelAftertouchMPE lands in the per-channel slot")
{
    sfz::Synth synth;
    synth.channelAftertouchMPE(0, 1, 64);
    synth.channelAftertouchMPE(0, 2, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getChannelAftertouch(1) == 64_norm);
    REQUIRE(mid.getChannelAftertouch(2) == 127_norm);
    REQUIRE(mid.getChannelAftertouch(0) == 0.0_a);
}

TEST_CASE("[MPE] Synth::polyAftertouchMPE lands in the per-channel slot")
{
    sfz::Synth synth;
    synth.polyAftertouchMPE(0, 1, 60, 64);
    synth.polyAftertouchMPE(0, 2, 60, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPolyAftertouch(1, 60) == 64_norm);
    REQUIRE(mid.getPolyAftertouch(2, 60) == 127_norm);
    REQUIRE(mid.getPolyAftertouch(0, 60) == 0.0_a);
}

TEST_CASE("[MPE] Existing single-channel API forwards to master-channel slot")
{
    sfz::Synth synth;
    // Using the legacy non-MPE API should populate channel 0 (master).
    synth.pitchWheel(0, 8192); // +1 normalized
    synth.cc(0, 74, 90);
    synth.channelAftertouch(0, 100);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPitchBend(0) == 1.0_a);
    REQUIRE(mid.getCCValue(0, 74) == 90_norm);
    REQUIRE(mid.getChannelAftertouch(0) == 100_norm);
    // Other channels untouched.
    REQUIRE(mid.getPitchBend(1) == 0.0_a);
    REQUIRE(mid.getCCValue(1, 74) == 0.0_a);
    REQUIRE(mid.getChannelAftertouch(1) == 0.0_a);
}

// =============================================================================
// Synth: noteOnMPE tags spawned voices with the originating channel
// =============================================================================

TEST_CASE("[MPE] noteOnMPE tags TriggerEvent with the dispatched channel")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_basic.sfz", R"(
        <region> sample=*sine
    )");

    synth.noteOnMPE(0, /*channel=*/1, 60, 100);
    synth.noteOnMPE(0, /*channel=*/3, 64, 100);
    synth.renderBlock(buffer);

    auto activeVoices = synth.getActiveVoices();
    REQUIRE(activeVoices.size() == 2);

    // Each voice's TriggerEvent should carry the channel it was triggered on.
    int sumChannels = 0;
    int sumNumbers = 0;
    for (const sfz::Voice* v : activeVoices) {
        sumChannels += v->getTriggerEvent().channel;
        sumNumbers += v->getTriggerEvent().number;
    }
    REQUIRE(sumChannels == 1 + 3);
    REQUIRE(sumNumbers == 60 + 64);

    // And the per-channel pairing must match (the ch=1 voice plays note 60,
    // ch=3 voice plays note 64).
    bool foundCh1Note60 = false;
    bool foundCh3Note64 = false;
    for (const sfz::Voice* v : activeVoices) {
        const auto& t = v->getTriggerEvent();
        if (t.channel == 1 && t.number == 60) foundCh1Note60 = true;
        if (t.channel == 3 && t.number == 64) foundCh3Note64 = true;
    }
    REQUIRE(foundCh1Note60);
    REQUIRE(foundCh3Note64);
}

// =============================================================================
// MPE configuration getters/setters
// =============================================================================

TEST_CASE("[MPE] setMPEEnabled / getMPEEnabled round-trip")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);
    synth.setMPEEnabled(true);
    REQUIRE(synth.getMPEEnabled() == true);
    synth.setMPEEnabled(false);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] setMPEPitchBendRange / getters round-trip")
{
    sfz::Synth synth;
    // MPE 1.0 default conventions.
    REQUIRE(synth.getMPEMasterPitchBendRange() == 2.0_a);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 48.0_a);

    synth.setMPEPitchBendRange(7.0f, 24.0f);
    REQUIRE(synth.getMPEMasterPitchBendRange() == 7.0_a);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 24.0_a);
}

// =============================================================================
// Voice stealing: same-channel preference under MPE
// =============================================================================

TEST_CASE("[MPE] Voice stealing prefers same-channel candidates when MPE enabled")
{
    // Setup: polyphony cap = 4, three voices on channel 2 plus one voice on
    // channel 1. Trigger a fifth note on channel 1 — with MPE enabled, the
    // stealer must pick the existing ch1 voice as the victim because it is
    // the only same-channel candidate. After the steal, all three channel-2
    // voices must still be alive.

    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setNumVoices(4);
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_stealing.sfz", R"(
        <region> sample=*sine
    )");

    synth.noteOnMPE(0, 2, 60, 100); synth.renderBlock(buffer);
    synth.noteOnMPE(0, 2, 62, 100); synth.renderBlock(buffer);
    synth.noteOnMPE(0, 2, 64, 100); synth.renderBlock(buffer);
    synth.noteOnMPE(0, 1, 67, 100); synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 4);

    synth.noteOnMPE(0, 1, 69, 100);
    synth.renderBlock(buffer);

    int channel2Count = 0;
    for (const sfz::Voice* v : synth.getActiveVoices())
        if (v->getTriggerEvent().channel == 2)
            channel2Count++;
    REQUIRE(channel2Count == 3);
}
