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

#pragma once
#include "CCMap.h"
#include "LeakDetector.h"
#include "Defaults.h"
#include "EGDescription.h"
#include "Opcode.h"
#include "AudioBuffer.h"
#include "MidiState.h"
#include "absl/strings/str_cat.h"
#include <bitset>
#include <absl/types/optional.h>
#include <random>
#include <string>
#include <vector>

namespace sfz {
/**
 * @brief Regions are the basic building blocks for the SFZ parsing and handling code.
 * All SFZ files are made of regions that are activated when a key is pressed or a CC
 * is triggered. Most opcodes constrain the situations in which a region can be activated.
 * Once activated, the Synth object will find a voice to play the region.
 *
 * This class is mostly open as there are a ton of parameters needed for the voice to be
 * able to play the region, and using getters would incur a ton of boilerplate code. Note
 * also that some parameters may be parsed and stored in regions but no playing logi is
 * available in the voices to take advantage of them.
 *
 */
struct Region {
    Region(const MidiState& midiState, absl::string_view defaultPath = "")
    : midiState(midiState), defaultPath(std::move(defaultPath))
    {
        ccSwitched.set();
    }
    Region(const Region&) = default;
    ~Region() = default;

    /**
     * @brief Triggers on release?
     *
     * @return true
     * @return false
     */
    bool isRelease() const noexcept { return trigger == SfzTrigger::release || trigger == SfzTrigger::release_key; }
    /**
     * @brief Is a generator (*sine or *silence mostly)?
     *
     * @return true
     * @return false
     */
    bool isGenerator() const noexcept { return sample.size() > 0 ? sample[0] == '*' : false; }
    /**
     * @brief Is a looping region (at least potentially)?
     *
     * @return true
     * @return false
     */
    bool shouldLoop() const noexcept { return (loopMode == SfzLoopMode::loop_continuous || loopMode == SfzLoopMode::loop_sustain); }
    /**
     * @brief Given the current midi state, is the region switched on?
     *
     * @return true
     * @return false
     */
    bool isSwitchedOn() const noexcept;
    /**
     * @brief Register a new note on event. The region may be switched on or off using keys so
     * this function updates the keyswitches state.
     *
     * @param noteNumber
     * @param velocity
     * @param randValue a random value between 0 and 1 used to randomize a bit the region activations
     *                  and vary the samples
     * @return true if the region should trigger on this event.
     * @return false
     */
    bool registerNoteOn(int noteNumber, uint8_t velocity, float randValue) noexcept;
    /**
     * @brief Register a new note off event. The region may be switched on or off using keys so
     * this function updates the keyswitches state.
     *
     * @param noteNumber
     * @param velocity
     * @param randValue a random value between 0 and 1 used to randomize a bit the region activations
     *                  and vary the samples
     * @return true if the region should trigger on this event.
     * @return false
     */
    bool registerNoteOff(int noteNumber, uint8_t velocity, float randValue) noexcept;
    /**
     * @brief Register a new CC event. The region may be switched on or off using CCs so
     * this function checks if it indeeds need to activate or not.
     *
     * @param ccNumber
     * @param ccValue
     * @return true if the region should trigger on this event
     * @return false
     */
    bool registerCC(int ccNumber, uint8_t ccValue) noexcept;
    /**
     * @brief Register a new pitch wheel event.
     *
     * @param pitch
     */
    void registerPitchWheel(int pitch) noexcept;
    /**
     * @brief Register a new aftertouch event.
     *
     * @param aftertouch
     */
    void registerAftertouch(uint8_t aftertouch) noexcept;
    /**
     * @brief Register tempo
     *
     * @param secondsPerQuarter
     */
    void registerTempo(float secondsPerQuarter) noexcept;

    /**
     * @brief Get the base pitch of the region depending on which note has been
     * pressed and at which velocity.
     *
     * @param noteNumber
     * @param velocity
     * @return float
     */
    float getBasePitchVariation(int noteNumber, uint8_t velocity) noexcept;
    /**
     * @brief Get the note-related gain of the region depending on which note has been
     * pressed and at which velocity.
     *
     * @param noteNumber
     * @param velocity
     * @return float
     */
    float getNoteGain(int noteNumber, uint8_t velocity) noexcept;
    /**
     * @brief Get the additional crossfade gain of the region depending on the
     * CC values
     *
     * @param ccState
     * @return float
     */
    float getCrossfadeGain(const SfzCCArray& ccState) noexcept;
    /**
     * @brief Get the base volume of the region depending on which note has been
     * pressed to trigger the region.
     *
     * @param noteNumber
     * @return float
     */
    float getBaseVolumedB(int noteNumber) noexcept;
    /**
     * @brief Get the base gain of the region.
     *
     * @return float
     */
    float getBaseGain() noexcept;
    /**
     * @brief Computes the gain value related to the velocity of the note
     *
     * @return float
     */
    float velocityCurve(uint8_t velocity) const noexcept;
    /**
     * @brief Get the region offset in samples
     *
     * @return uint32_t
     */
    uint32_t getOffset(Oversampling factor = Oversampling::x1) noexcept;
    /**
     * @brief Get the region delay in seconds
     *
     * @return float
     */
    float getDelay() noexcept;
    /**
     * @brief Get the index of the sample end, either natural end or forced
     * loop.
     *
     * @return uint32_t
     */
    uint32_t trueSampleEnd(Oversampling factor = Oversampling::x1) const noexcept;
    /**
     * @brief Parse a new opcode into the region to fill in the proper parameters.
     * This must be called multiple times for each opcode applying to this region.
     *
     * @param opcode
     * @return true if the opcode was properly read and stored.
     * @return false
     */
    bool parseOpcode(const Opcode& opcode);

    void offsetAllKeys(int offset) noexcept;

    uint32_t loopStart(Oversampling factor = Oversampling::x1) const noexcept;
    uint32_t loopEnd(Oversampling factor = Oversampling::x1) const noexcept;

    // Sound source: sample playback
    std::string sample {}; // Sample
    float delay { Default::delay }; // delay
    float delayRandom { Default::delayRandom }; // delay_random
    uint32_t offset { Default::offset }; // offset
    uint32_t offsetRandom { Default::offsetRandom }; // offset_random
    uint32_t sampleEnd { Default::sampleEndRange.getEnd() }; // end
    absl::optional<uint32_t> sampleCount {}; // count
    absl::optional<SfzLoopMode> loopMode {}; // loopmode
    Range<uint32_t> loopRange { Default::loopRange }; //loopstart and loopend

    // Instrument settings: voice lifecycle
    uint32_t group { Default::group }; // group
    absl::optional<uint32_t> offBy {}; // off_by
    SfzOffMode offMode { Default::offMode }; // off_mode

    // Region logic: key mapping
    Range<uint8_t> keyRange { Default::keyRange }; //lokey, hikey and key
    Range<uint8_t> velocityRange { Default::velocityRange }; // hivel and lovel

    // Region logic: MIDI conditions
    Range<int> bendRange { Default::bendRange }; // hibend and lobend
    CCMap<Range<uint8_t>> ccConditions { Default::ccValueRange };
    Range<uint8_t> keyswitchRange { Default::keyRange }; // sw_hikey and sw_lokey
    absl::optional<uint8_t> keyswitch {}; // sw_last
    absl::optional<uint8_t> keyswitchUp {}; // sw_up
    absl::optional<uint8_t> keyswitchDown {}; // sw_down
    absl::optional<uint8_t> previousNote {}; // sw_previous
    SfzVelocityOverride velocityOverride { Default::velocityOverride }; // sw_vel
    bool checkSustain { Default::checkSustain }; // sustain_sw
    bool checkSostenuto { Default::checkSostenuto }; // sostenuto_sw

    // Region logic: internal conditions
    Range<uint8_t> aftertouchRange { Default::aftertouchRange }; // hichanaft and lochanaft
    Range<float> bpmRange { Default::bpmRange }; // hibpm and lobpm
    Range<float> randRange { Default::randRange }; // hirand and lorand
    uint8_t sequenceLength { Default::sequenceLength }; // seq_length
    uint8_t sequencePosition { Default::sequencePosition }; // seq_position

    // Region logic: triggers
    SfzTrigger trigger { Default::trigger }; // trigger
    CCMap<Range<uint8_t>> ccTriggers { Default::ccTriggerValueRange }; // on_loccN on_hiccN

    // Performance parameters: amplifier
    float volume { Default::volume }; // volume
    float amplitude { Default::amplitude }; // amplitude
    float pan { Default::pan }; // pan
    float width { Default::width }; // width
    float position { Default::position }; // position
    absl::optional<CCValuePair> volumeCC; // volume_oncc
    absl::optional<CCValuePair> amplitudeCC; // amplitude_oncc
    absl::optional<CCValuePair> panCC; // pan_oncc
    absl::optional<CCValuePair> widthCC; // width_oncc
    absl::optional<CCValuePair> positionCC; // position_oncc
    uint8_t ampKeycenter { Default::ampKeycenter }; // amp_keycenter
    float ampKeytrack { Default::ampKeytrack }; // amp_keytrack
    float ampVeltrack { Default::ampVeltrack }; // amp_keytrack
    std::vector<std::pair<int, float>> velocityPoints; // amp_velcurve_N
    float ampRandom { Default::ampRandom }; // amp_random
    Range<uint8_t> crossfadeKeyInRange { Default::crossfadeKeyInRange };
    Range<uint8_t> crossfadeKeyOutRange { Default::crossfadeKeyOutRange };
    Range<uint8_t> crossfadeVelInRange { Default::crossfadeVelInRange };
    Range<uint8_t> crossfadeVelOutRange { Default::crossfadeVelOutRange };
    SfzCrossfadeCurve crossfadeKeyCurve { Default::crossfadeKeyCurve };
    SfzCrossfadeCurve crossfadeVelCurve { Default::crossfadeVelCurve };
    SfzCrossfadeCurve crossfadeCCCurve { Default::crossfadeCCCurve };
    CCMap<Range<uint8_t>> crossfadeCCInRange { Default::crossfadeCCInRange }; // xfin_loccN xfin_hiccN
    CCMap<Range<uint8_t>> crossfadeCCOutRange { Default::crossfadeCCOutRange }; // xfout_loccN xfout_hiccN
    float rtDecay { Default::rtDecay }; // rt_decay


    // Performance parameters: pitch
    uint8_t pitchKeycenter { Default::pitchKeycenter }; // pitch_keycenter
    int pitchKeytrack { Default::pitchKeytrack }; // pitch_keytrack
    int pitchRandom { Default::pitchRandom }; // pitch_random
    int pitchVeltrack { Default::pitchVeltrack }; // pitch_veltrack
    int transpose { Default::transpose }; // transpose
    int tune { Default::tune }; // tune
    int bendUp { Default::bendUp };
    int bendDown { Default::bendDown };
    int bendStep { Default::bendStep };

    // Envelopes
    EGDescription amplitudeEG;
    EGDescription pitchEG;
    EGDescription filterEG;

    bool isStereo { false };
private:
    const MidiState& midiState;
    bool keySwitched { true };
    bool previousKeySwitched { true };
    bool sequenceSwitched { true };
    bool pitchSwitched { true };
    bool bpmSwitched { true };
    bool aftertouchSwitched { true };
    std::bitset<config::numCCs> ccSwitched;
    bool triggerOnCC { false };
    absl::string_view defaultPath { "" };

    int sequenceCounter { 0 };

    std::uniform_real_distribution<float> volumeDistribution { -sfz::Default::ampRandom, sfz::Default::ampRandom };
    std::uniform_real_distribution<float> delayDistribution { 0, sfz::Default::delayRandom };
    std::uniform_int_distribution<uint32_t> offsetDistribution { 0, sfz::Default::offsetRandom };
    std::uniform_int_distribution<int> pitchDistribution { -sfz::Default::pitchRandom, sfz::Default::pitchRandom };
    LEAK_DETECTOR(Region);
};

} // namespace sfz
