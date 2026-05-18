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

TEST_CASE("[MPE] Synth::pitchWheel lands in the per-channel pitch slot")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true); // *MPE methods honor the channel arg only when MPE is on
    synth.pitchWheel(0, 1, 4096);
    synth.pitchWheel(0, 2, -4096);

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

TEST_CASE("[MPE] Synth::cc lands in the per-channel CC slot")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, 1, 74, 64);
    synth.cc(0, 2, 74, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(1, 74) == 64_norm);
    REQUIRE(mid.getCCValue(2, 74) == 127_norm);
    REQUIRE(mid.getCCValue(0, 74) == 0.0_a);
}

TEST_CASE("[MPE] Synth::channelAftertouch lands in the per-channel slot")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.channelAftertouch(0, 1, 64);
    synth.channelAftertouch(0, 2, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getChannelAftertouch(1) == 64_norm);
    REQUIRE(mid.getChannelAftertouch(2) == 127_norm);
    REQUIRE(mid.getChannelAftertouch(0) == 0.0_a);
}

TEST_CASE("[MPE] Synth::polyAftertouch on Manager Channel lands in the per-channel slot")
{
    // Member-channel Poly KP is dropped by the MPE 1.0 §2.2.7 filter, so the
    // per-channel storage path is only exercisable through the Manager
    // Channel (channel 0) under MPE on. The MidiState-direct test above
    // covers raw per-channel writes for channels 1..15.
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.polyAftertouch(0, 0, 60, 64);
    synth.polyAftertouch(0, 0, 64, 127);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPolyAftertouch(0, 60) == 64_norm);
    REQUIRE(mid.getPolyAftertouch(0, 64) == 127_norm);
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

TEST_CASE("[MPE] When MPE is disabled, *MPE methods collapse channel to 0 (single-channel contract)")
{
    // With MPE off, the *MPE API surface and the legacy channel-less API
    // are equivalent: both land in channel-0 storage and both trigger
    // voices with triggerChannel_=0. Consumers (the VST3 / LV2 wrappers
    // in sfizz-ui, or any downstream embedder) can therefore call *MPE
    // unconditionally without an MPE-off vs MPE-on dispatch branch, and
    // the engine takes sole responsibility for what "MPE off" means.
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/MPE_off_normalization.sfz", R"(
        <region> sample=*sine
    )");

    // *MPE noteOn on a non-zero channel — voice should still get
    // triggerChannel_=0 because MPE is off.
    synth.noteOn(0, /*channel=*/5, 60, 100);
    synth.renderBlock(buffer);

    auto activeVoices = synth.getActiveVoices();
    REQUIRE(activeVoices.size() == 1);
    REQUIRE(activeVoices[0]->getTriggerEvent().channel == 0);

    // *MPE modulation writes on non-zero channels also land in channel 0.
    synth.pitchWheel(0, /*channel=*/5, 4096);
    synth.cc(0, /*channel=*/5, 74, 90);
    synth.channelAftertouch(0, /*channel=*/5, 100);
    synth.renderBlock(buffer);

    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPitchBendRaw(0) == Approx(0.5).margin(0.001));
    REQUIRE(mid.getCCValue(0, 74) == 90_norm);
    REQUIRE(mid.getChannelAftertouch(0) == 100_norm);
    // Channel 5 should NOT have any of these writes — they were normalized.
    REQUIRE(mid.getPitchBendRaw(5) == 0.0_a);

    // *MPE noteOff on a non-zero channel also collapses to channel 0 and
    // matches the voice (which has triggerChannel_=0).
    synth.noteOff(0, /*channel=*/5, 60, 0);
    synth.renderBlock(buffer);
    REQUIRE((activeVoices[0]->released() || activeVoices[0]->isFree()));
}

TEST_CASE("[MPE] setMPEEnabled(false) flushes active voices triggered while MPE was on")
{
    // On→off transition has to clear voices that carry triggerChannel_>0,
    // otherwise their channel-aware modulation reads stay pinned to the
    // member channel that no longer receives input under the new contract
    // (everything collapses to channel 0 after the flip).
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/MPE_off_transition.sfz", R"(
        <region> sample=*sine
    )");

    synth.noteOn(0, /*channel=*/3, 60, 100);
    synth.noteOn(0, /*channel=*/4, 64, 100);
    synth.renderBlock(buffer);
    REQUIRE(synth.getActiveVoices().size() == 2);

    synth.setMPEEnabled(false);
    synth.renderBlock(buffer);

    // All voices should have been released by allSoundOff() in setMPEEnabled.
    for (const sfz::Voice* v : synth.getActiveVoices()) {
        REQUIRE(v->isFree());
    }
}

// =============================================================================
// Synth: noteOn tags spawned voices with the originating channel
// =============================================================================

TEST_CASE("[MPE] noteOn tags TriggerEvent with the dispatched channel")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_basic.sfz", R"(
        <region> sample=*sine
    )");

    synth.noteOn(0, /*channel=*/1, 60, 100);
    synth.noteOn(0, /*channel=*/3, 64, 100);
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

    synth.noteOn(0, 2, 60, 100); synth.renderBlock(buffer);
    synth.noteOn(0, 2, 62, 100); synth.renderBlock(buffer);
    synth.noteOn(0, 2, 64, 100); synth.renderBlock(buffer);
    synth.noteOn(0, 1, 67, 100); synth.renderBlock(buffer);
    REQUIRE(synth.getNumActiveVoices() == 4);

    synth.noteOn(0, 1, 69, 100);
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
    synth.cc(0, channel, 101, 0);          // RPN MSB
    synth.cc(0, channel, 100, 6);          // RPN LSB → RPN 6
    synth.cc(0, channel, 6,   memberCount); // Data Entry MSB
    synth.cc(0, channel, 101, 127);        // Null RPN MSB
    synth.cc(0, channel, 100, 127);        // Null RPN LSB
}

void sendPitchBendSensitivity(sfz::Synth& synth, int channel, int semitones)
{
    synth.cc(0, channel, 101, 0);          // RPN MSB
    synth.cc(0, channel, 100, 0);          // RPN LSB → RPN 0
    synth.cc(0, channel, 6,   semitones);  // Data Entry MSB
    synth.cc(0, channel, 101, 127);        // Null RPN MSB
    synth.cc(0, channel, 100, 127);        // Null RPN LSB
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
    synth.cc(0, 0, 101, 127);
    synth.cc(0, 0, 100, 127);
    // A bare Data Entry with no RPN selected must not flip MPE state.
    synth.cc(0, 0, 6, 8);
    REQUIRE(synth.getMPEEnabled() == false);
}

TEST_CASE("[MPE] NRPN sequence followed by CC 6 does not trigger MPE handlers")
{
    sfz::Synth synth;
    // Select NRPN (0, 6) on the master channel — same data values as RPN 6
    // but via CC 99 / CC 98 instead of CC 101 / CC 100. The parser must not
    // mistake this for an MCM.
    synth.cc(0, 0, 99, 0);
    synth.cc(0, 0, 98, 6);
    synth.cc(0, 0, 6, 8);
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
    synth.cc(0, 0, 101, 0);
    synth.cc(0, 0, 100, 6);
    synth.cc(0, 0, 6,   8);

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

// =============================================================================
// Polyphonic Key Pressure asymmetry (MPE 1.0 §2.2.7 / Appendix E Table 5)
// =============================================================================
//
// Poly KP is prohibited on Member Channels under MPE — per-note pressure
// flows through Channel Pressure, and Poly KP on a Member Channel would
// compound the response. The engine drops such events at hdPolyAftertouch
// and reports the drop via getDroppedPolyKpOnMemberCount. Manager-Channel
// Poly KP remains permitted (spec leaves this to the implementer for
// compatibility with non-MPE-aware devices) and still routes to MidiState.

TEST_CASE("[MPE] Poly KP on a Member Channel is dropped when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    REQUIRE(synth.getDroppedPolyKpOnMemberCount() == 0);

    synth.polyAftertouch(0, /*channel=*/2, /*note=*/60, 100);

    REQUIRE(synth.getDroppedPolyKpOnMemberCount() == 1);
    // The dropped event must not reach MidiState — the slot stays at the
    // default 0 / master-fallback value.
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPolyAftertouch(/*channel=*/2, /*note=*/60) == 0.0_a);
}

TEST_CASE("[MPE] Poly KP on the Manager Channel still routes through when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    REQUIRE(synth.getDroppedPolyKpOnMemberCount() == 0);

    synth.polyAftertouch(0, /*channel=*/0, /*note=*/60, 127);

    REQUIRE(synth.getDroppedPolyKpOnMemberCount() == 0);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPolyAftertouch(/*channel=*/0, /*note=*/60) == 127_norm);
}

TEST_CASE("[MPE] Poly KP on any channel is accepted when MPE is disabled")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);

    synth.polyAftertouch(0, /*channel=*/5, /*note=*/72, 80);

    REQUIRE(synth.getDroppedPolyKpOnMemberCount() == 0);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPolyAftertouch(/*channel=*/5, /*note=*/72) == 80_norm);
}

// =============================================================================
// Manager-only message filtering (MPE 1.0 §2.3.1 / §2.3.3 / Appendix E Table 5)
// =============================================================================
//
// Pedal CCs (64-69), mode/reset CCs (120-125 excluding 122) and Bank Select
// (CC 0 / CC 32) are zone-wide and must be honoured only on the Manager
// Channel when MPE is enabled. The engine drops such events at the top of
// performHdcc and reports the drop via getDroppedManagerOnlyMessageCount.
// RPN data CCs (6/38/98/99/100/101) are NOT zone-wide — they are per-channel
// state machines — and must continue to flow on Member Channels so the
// RPN 0 (Pitch Bend Sensitivity) auto-config keeps working.

TEST_CASE("[MPE] Damper on the Manager Channel is registered when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, /*channel=*/0, /*ccNumber=*/64, 127);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 0);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(/*channel=*/0, 64) == 127_norm);
}

TEST_CASE("[MPE] Damper on a Member Channel is dropped when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, /*channel=*/2, /*ccNumber=*/64, 127);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 1);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(/*channel=*/2, 64) == 0.0_a);
}

TEST_CASE("[MPE] All pedal CCs 64-69 drop on Member Channels under MPE")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    for (int cc : {64, 65, 66, 67, 68, 69})
        synth.cc(0, /*channel=*/3, cc, 127);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 6);
}

TEST_CASE("[MPE] All Notes Off on a Member Channel is dropped when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, /*channel=*/2, /*ccNumber=*/123, 0);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 1);
    // The CC must not reach MidiState even though All-Notes-Off normally
    // hits a global early-return path in performHdcc.
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(/*channel=*/2, 123) == 0.0_a);
}

TEST_CASE("[MPE] Reset All Controllers on a Member Channel is dropped when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, /*channel=*/4, /*ccNumber=*/121, 0);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 1);
}

TEST_CASE("[MPE] Bank Select MSB/LSB on a Member Channel is dropped when MPE is enabled")
{
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    synth.cc(0, /*channel=*/2, /*ccNumber=*/0, 5);   // Bank MSB
    synth.cc(0, /*channel=*/2, /*ccNumber=*/32, 3);  // Bank LSB
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 2);
}

TEST_CASE("[MPE] Manager-only filter is inert when MPE is disabled")
{
    sfz::Synth synth;
    REQUIRE(synth.getMPEEnabled() == false);
    synth.cc(0, /*channel=*/2, /*ccNumber=*/64, 127);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 0);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(/*channel=*/2, 64) == 127_norm);
}

TEST_CASE("[MPE] Manager-only filter does not touch RPN data CCs on Member Channels")
{
    // Regression for the RPN parser tap: the per-note bend range
    // auto-config must keep working when MPE is enabled and the RPN
    // sequence arrives on a Member Channel. CCs 6 / 38 / 98 / 99 / 100 /
    // 101 must remain off the Manager-only list.
    sfz::Synth synth;
    synth.setMPEEnabled(true);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 48.0_a);
    sendPitchBendSensitivity(synth, /*channel=*/2, /*semitones=*/24);
    REQUIRE(synth.getMPEPerNotePitchBendRange() == 24.0_a);
    REQUIRE(synth.getDroppedManagerOnlyMessageCount() == 0);
}

// =============================================================================
// Released-note expression filter (MPE 1.0 §2.2.6 / §2.2.7 / §2.2.8)
// =============================================================================
//
// Per the spec, a Released Note stops reacting to per-finger Pitch Bend,
// Channel Pressure and CC#74 on its Member Channel after the Note Off
// message occurs — because the controller will recycle that Member Channel
// for the next finger. Manager-Channel traffic still reaches the release
// tail (§A.4.1). Voice::expressionChannel() returns the channel that
// expression reads should consult: the trigger channel for active voices
// (and for any voice triggered on the Manager Channel), and channel 0
// (Lower Zone Manager) for released voices that were triggered on a
// Member Channel. Every per-block read site (pitch envelope, the CC /
// Channel Pressure / Poly Aftertouch mod sources, and region crossfades)
// routes through this helper so the redirect applies uniformly.

TEST_CASE("[MPE] expressionChannel: active voice on Member Channel uses its trigger channel")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->released() == false);
    REQUIRE(active[0]->expressionChannel() == 2);
}

TEST_CASE("[MPE] expressionChannel: released voice on Member Channel redirects to Manager")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);
    synth.noteOff(0, /*channel=*/2, /*note=*/60, 0);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->released() == true);
    REQUIRE(active[0]->expressionChannel() == 0);
    // The trigger channel itself is unchanged — only the expression read
    // channel redirects. Useful invariant for voice stealing / debugging.
    REQUIRE(active[0]->getTriggerEvent().channel == 2);
}

TEST_CASE("[MPE] expressionChannel: voice on Manager Channel always reads Manager")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/0, /*note=*/60, 100);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->expressionChannel() == 0);

    synth.noteOff(0, /*channel=*/0, /*note=*/60, 0);
    synth.renderBlock(buffer);
    REQUIRE(active[0]->released() == true);
    REQUIRE(active[0]->expressionChannel() == 0);
}

TEST_CASE("[MPE] Pitch Bend on released Member Channel doesn't reach the voice's expression read")
{
    // The released voice's expression reads route through Manager (ch 0),
    // so a per-finger Pitch Bend that lands in MidiState's ch 2 slot
    // doesn't influence the released voice. Manager-Channel PB still does.
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);
    synth.noteOff(0, /*channel=*/2, /*note=*/60, 0);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->expressionChannel() == 0);

    synth.hdPitchWheel(0, /*channel=*/2, 0.5f);
    auto& mid = synth.getResources().getMidiState();
    // PB lands in MidiState ch 2 ...
    REQUIRE(mid.getPitchBendRaw(2) == 0.5f);
    // ... but the voice now reads from ch 0 (master), where no PB lives.
    REQUIRE(mid.getPitchBendRaw(0) == 0.0f);
    REQUIRE(active[0]->expressionChannel() == 0);
}

TEST_CASE("[MPE] Channel Pressure on released Member Channel doesn't reach the voice's expression read")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);
    synth.noteOff(0, /*channel=*/2, /*note=*/60, 0);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->expressionChannel() == 0);

    synth.channelAftertouch(0, /*channel=*/2, 100);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getChannelAftertouch(/*channel=*/2) == 100_norm);
    REQUIRE(mid.getChannelAftertouch(/*channel=*/0) == 0.0_a);
    REQUIRE(active[0]->expressionChannel() == 0);
}

TEST_CASE("[MPE] CC74 on released Member Channel doesn't reach the voice's expression read")
{
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);
    synth.noteOff(0, /*channel=*/2, /*note=*/60, 0);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->expressionChannel() == 0);

    synth.cc(0, /*channel=*/2, /*ccNumber=*/74, 90);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getCCValue(/*channel=*/2, 74) == 90_norm);
    REQUIRE(mid.getCCValue(/*channel=*/0, 74) == 0.0_a);
    REQUIRE(active[0]->expressionChannel() == 0);
}

TEST_CASE("[MPE] Active voice on Member Channel still responds to per-finger expression")
{
    // Regression: the released-note gate must not bleed into active
    // voices. The expression channel of an active member-channel voice
    // is its trigger channel; PB / CP / CC74 sent on that channel reach
    // the voice as today.
    sfz::Synth synth;
    sfz::AudioBuffer<float> buffer { 2, static_cast<unsigned>(synth.getSamplesPerBlock()) };
    synth.setMPEEnabled(true);
    synth.loadSfzString(fs::current_path() / "tests/TestFiles/mpe_released.sfz", R"(
        <region> sample=*sine ampeg_release=1
    )");
    synth.noteOn(0, /*channel=*/2, /*note=*/60, 100);
    synth.renderBlock(buffer);

    auto active = synth.getActiveVoices();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0]->released() == false);
    REQUIRE(active[0]->expressionChannel() == 2);

    synth.hdPitchWheel(0, /*channel=*/2, 0.25f);
    synth.channelAftertouch(0, /*channel=*/2, 80);
    synth.cc(0, /*channel=*/2, 74, 70);
    auto& mid = synth.getResources().getMidiState();
    REQUIRE(mid.getPitchBendRaw(2) == 0.25f);
    REQUIRE(mid.getChannelAftertouch(2) == 80_norm);
    REQUIRE(mid.getCCValue(2, 74) == 70_norm);
    REQUIRE(active[0]->expressionChannel() == 2);
}
