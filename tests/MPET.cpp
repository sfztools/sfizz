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

TEST_CASE("[MPE] Empty member channels inherit master CC / pitch / aftertouch state")
{
    // Regression: hit during Osmose hand-test. sfizz seeds default values
    // for CC7 (Volume@~0.79), CC10 (Pan@0.5), CC11 (Expression@1.0) into
    // the master channel only. Without inheritance, voices on member
    // channels saw CC7=0 / CC11=0 and rendered near-silent — the user
    // perceived "first note plays at low volume, then mutes".
    sfz::MidiState state;
    state.ccEvent(0, /*channel=*/0, 7, 0.79f);
    state.ccEvent(0, /*channel=*/0, 11, 1.0f);
    state.pitchBendEvent(0, /*channel=*/0, 0.25f);
    state.channelAftertouchEvent(0, /*channel=*/0, 0.6f);
    state.polyAftertouchEvent(0, /*channel=*/0, 60, 0.4f);

    // Member channel 5 has never received its own values.
    REQUIRE(state.getCCEvents(5, 7).back().value == 0.79f);
    REQUIRE(state.getCCEvents(5, 11).back().value == 1.0f);
    REQUIRE(state.getPitchEvents(5).back().value == 0.25f);
    REQUIRE(state.getChannelAftertouchEvents(5).back().value == 0.6f);
    REQUIRE(state.getPolyAftertouchEvents(5, 60).back().value == 0.4f);

    // Once the member channel writes its own value, it overrides master.
    state.ccEvent(0, /*channel=*/5, 7, 0.3f);
    REQUIRE(state.getCCEvents(5, 7).back().value == 0.3f);
    // Master is unchanged.
    REQUIRE(state.getCCEvents(0, 7).back().value == 0.79f);
}

TEST_CASE("[MPE] First member-channel event at delay>0 keeps the delay-0 sentinel")
{
    // Regression: hit during Osmose hand-test. linearEnvelope ASSERTs the
    // event vector starts at delay 0; member channels are populated lazily
    // so before this fix a first write at delay>0 produced a vector whose
    // first entry was {delay, value}, tripping the ASSERT and SIGTRAP'ing
    // the audio thread on the very first MPE pitch-bend / CC event.
    sfz::MidiState state;

    state.pitchBendEvent(/*delay=*/42, /*channel=*/3, 0.5f);
    state.ccEvent(/*delay=*/17, /*channel=*/4, 74, 0.7f);
    state.channelAftertouchEvent(/*delay=*/9, /*channel=*/5, 0.4f);
    state.polyAftertouchEvent(/*delay=*/3, /*channel=*/6, 60, 0.6f);

    auto firstDelayIsZero = [] (const sfz::EventVector& v) {
        return ! v.empty() && v.front().delay == 0;
    };

    REQUIRE(firstDelayIsZero(state.getPitchEvents(3)));
    REQUIRE(firstDelayIsZero(state.getCCEvents(4, 74)));
    REQUIRE(firstDelayIsZero(state.getChannelAftertouchEvents(5)));
    REQUIRE(firstDelayIsZero(state.getPolyAftertouchEvents(6, 60)));

    // The original event is still there, just preceded by the sentinel.
    REQUIRE(state.getPitchEvents(3).size() == 2);
    REQUIRE(state.getPitchEvents(3).back().delay == 42);
    REQUIRE(state.getPitchEvents(3).back().value == 0.5f);
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
    // Member channels with no events of their own inherit master state
    // via the MidiState fallback (b117153f) — voices triggered on a member
    // channel before that channel sees any per-note modulation read the
    // master scalars so global bend / pressure / CC values still apply.
    REQUIRE(mid.getPitchBend(1) == 1.0_a);
    REQUIRE(mid.getCCValue(1, 74) == 90_norm);
    REQUIRE(mid.getChannelAftertouch(1) == 100_norm);
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

// =============================================================================
// MPE auto-config: RPN 6 (MCM) + RPN 0 (Pitch Bend Sensitivity) parsing
// =============================================================================
//
// MPE 1.0 §2 lets controllers self-announce their zone (RPN 6) and bend
// ranges (RPN 0). The engine parses these RPN sequences inside performHdcc
// and reacts automatically. The CCs themselves still propagate to MidiState
// so SFZ *_oncc bindings on CC 6/100/101/etc. remain functional.
//
// Lower-Zone master in sfizz's 0-indexed channel convention is channel 0
// (MIDI channel 1 on the wire); members are channels 1..15.

namespace {

void sendMCM(sfz::Synth& synth, int channel, int memberCount)
{
    synth.ccMPE(0, channel, 101, 0);          // RPN MSB
    synth.ccMPE(0, channel, 100, 6);          // RPN LSB → RPN 6
    synth.ccMPE(0, channel, 6,   memberCount); // Data Entry MSB
    synth.ccMPE(0, channel, 101, 127);        // Null RPN MSB
    synth.ccMPE(0, channel, 100, 127);        // Null RPN LSB
}

void sendPitchBendSensitivity(sfz::Synth& synth, int channel, int semitones)
{
    synth.ccMPE(0, channel, 101, 0);          // RPN MSB
    synth.ccMPE(0, channel, 100, 0);          // RPN LSB → RPN 0
    synth.ccMPE(0, channel, 6,   semitones);  // Data Entry MSB
    synth.ccMPE(0, channel, 101, 127);        // Null RPN MSB
    synth.ccMPE(0, channel, 100, 127);        // Null RPN LSB
}

} // namespace

TEST_CASE("[MPE] RPN 6 (MCM) on master channel enables MPE")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);
    sendMCM(synth, /*channel=*/0, /*memberCount=*/8);
    REQUIRE(synth.getMPEEnabled() == true);
}

TEST_CASE("[MPE] RPN 6 (MCM) with N=0 disables MPE")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    sendMCM(synth, /*channel=*/0, /*memberCount=*/0);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] RPN 6 (MCM) on non-master channel is rejected")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);
    sendMCM(synth, /*channel=*/5, /*memberCount=*/8);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] RPN 0 on master channel updates master bend range")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEMasterPitchBendRange() == 2.0_a);
    sendPitchBendSensitivity(synth, /*channel=*/0, /*semitones=*/12);
    REQUIRE(synth.getMPEMasterPitchBendRange() == 12.0_a);
    // Per-note range untouched.
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 48.0_a);
}

TEST_CASE("[MPE] RPN 0 on member channel updates per-note bend range")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 48.0_a);
    sendPitchBendSensitivity(synth, /*channel=*/2, /*semitones=*/24);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 24.0_a);
    // Master range untouched.
    REQUIRE(synth.getMPEMasterPitchBendRange() == 2.0_a);
}

TEST_CASE("[MPE] Null RPN followed by CC 6 does not trigger MPE handlers")
{
    sfz::Synth synth;
    // Deselect any pending RPN first.
    synth.ccMPE(0, 0, 101, 127);
    synth.ccMPE(0, 0, 100, 127);
    // A bare Data Entry with no RPN selected must not flip MPE state.
    synth.ccMPE(0, 0, 6, 8);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] NRPN sequence followed by CC 6 does not trigger MPE handlers")
{
    sfz::Synth synth;
    // Select NRPN (0, 6) on the master channel — same data values as RPN 6
    // but via CC 99 / CC 98 instead of CC 101 / CC 100. The parser must not
    // mistake this for an MCM.
    synth.ccMPE(0, 0, 99, 0);
    synth.ccMPE(0, 0, 98, 6);
    synth.ccMPE(0, 0, 6, 8);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] Opt-out: master-bend auto-config disabled blocks RPN 0 on master")
{
    sfz::Synth synth;
    synth.setMPEMasterBendAutoConfigEnabled(false);
    sendPitchBendSensitivity(synth, /*channel=*/0, /*semitones=*/12);
    REQUIRE(synth.getMPEMasterPitchBendRange() == 2.0_a);
    // The per-note opt-out is still on, so a member-channel RPN still lands.
    sendPitchBendSensitivity(synth, /*channel=*/2, /*semitones=*/24);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 24.0_a);
}

TEST_CASE("[MPE] Opt-out: per-note-bend auto-config disabled blocks RPN 0 on members")
{
    sfz::Synth synth;
    synth.setMPEPerNoteBendAutoConfigEnabled(false);
    sendPitchBendSensitivity(synth, /*channel=*/2, /*semitones=*/24);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 48.0_a);
    // The master opt-out is still on, so a master-channel RPN still lands.
    sendPitchBendSensitivity(synth, /*channel=*/0, /*semitones=*/12);
    REQUIRE(synth.getMPEMasterPitchBendRange() == 12.0_a);
}

TEST_CASE("[MPE] MCM enable is unconditional (not gated by the bend-range opt-outs)")
{
    sfz::Synth synth;
    synth.setMPEMasterBendAutoConfigEnabled(false);
    synth.setMPEPerNoteBendAutoConfigEnabled(false);
    sendMCM(synth, /*channel=*/0, /*memberCount=*/8);
    // MCM enable is part of the MPE 1.0 spec contract — UIs gate only the
    // bend-range updates, not the MPE-enable flag.
    REQUIRE(synth.getMPEEnabled() == true);
}

TEST_CASE("[MPE] RPN control CCs still propagate to MidiState (parser is a tap)")
{
    sfz::Synth synth;
    synth.ccMPE(0, 0, 101, 0);
    synth.ccMPE(0, 0, 100, 6);
    synth.ccMPE(0, 0, 6,   8);

    auto& mid = synth.getResources().getMidiState();
    // The parser must not swallow the CCs — SFZ instruments can bind
    // *_oncc6 / *_oncc100 / *_oncc101 and those bindings rely on the
    // MidiState slot being updated.
    REQUIRE(mid.getCCValue(0, 101) == 0.0_a);
    REQUIRE(mid.getCCValue(0, 100) == 6_norm);
    REQUIRE(mid.getCCValue(0, 6)   == 8_norm);
}

TEST_CASE("[MPE] Opt-out flag round-trip getters")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEMasterBendAutoConfigEnabled() == true);
    REQUIRE(synth.getMPEPerNoteBendAutoConfigEnabled() == true);
    synth.setMPEMasterBendAutoConfigEnabled(false);
    REQUIRE(synth.getMPEMasterBendAutoConfigEnabled() == false);
    REQUIRE(synth.getMPEPerNoteBendAutoConfigEnabled() == true);
    synth.setMPEPerNoteBendAutoConfigEnabled(false);
    REQUIRE(synth.getMPEPerNoteBendAutoConfigEnabled() == false);
}
