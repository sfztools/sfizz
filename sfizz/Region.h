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
#include <bitset>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace sfz {
struct Region {
    Region()
    {
        ccSwitched.set();
    }
    Region(const Region&) = default;
    ~Region() = default;

    bool isRelease() const noexcept { return trigger == SfzTrigger::release || trigger == SfzTrigger::release_key; }
    bool isGenerator() const noexcept { return sample.size() > 0 ? sample[0] == '*' : false; }
    bool shouldLoop() const noexcept { return (loopMode == SfzLoopMode::loop_continuous || loopMode == SfzLoopMode::loop_sustain); }
    bool isSwitchedOn() const noexcept;
    bool registerNoteOn(int channel, int noteNumber, uint8_t velocity, float randValue) noexcept;
    bool registerNoteOff(int channel, int noteNumber, uint8_t velocity, float randValue) noexcept;
    bool registerCC(int channel, int ccNumber, uint8_t ccValue) noexcept;
    void registerPitchWheel(int channel, int pitch) noexcept;
    void registerAftertouch(int channel, uint8_t aftertouch) noexcept;
    void registerTempo(float secondsPerQuarter) noexcept;
    bool isStereo() const noexcept;
    float getBasePitchVariation(int noteNumber, uint8_t velocity) noexcept;
    float getNoteGain(int noteNumber, uint8_t velocity) noexcept;
    float getCCGain(const CCValueArray& ccState) noexcept;
    float getBaseGain() noexcept;
    float velocityGain(uint8_t velocity) const noexcept;
    uint32_t getOffset() noexcept;
    uint32_t getDelay() noexcept;
    uint32_t trueSampleEnd() const noexcept;
    bool canUsePreloadedData() const noexcept;
    bool parseOpcode(const Opcode& opcode);

    // Sound source: sample playback
    std::string sample {}; // Sample
    float delay { Default::delay }; // delay
    float delayRandom { Default::delayRandom }; // delay_random
    uint32_t offset { Default::offset }; // offset
    uint32_t offsetRandom { Default::offsetRandom }; // offset_random
    uint32_t sampleEnd { Default::sampleEndRange.getEnd() }; // end
    std::optional<uint32_t> sampleCount {}; // count
    SfzLoopMode loopMode { Default::loopMode }; // loopmode
    Range<uint32_t> loopRange { Default::loopRange }; //loopstart and loopend

    // Instrument settings: voice lifecycle
    uint32_t group { Default::group }; // group
    std::optional<uint32_t> offBy {}; // off_by
    SfzOffMode offMode { Default::offMode }; // off_mode

    // Region logic: key mapping
    Range<uint8_t> keyRange { Default::keyRange }; //lokey, hikey and key
    Range<uint8_t> velocityRange { Default::velocityRange }; // hivel and lovel

    // Region logic: MIDI conditions
    Range<uint8_t> channelRange { Default::channelRange }; //lochan and hichan
    Range<int> bendRange { Default::bendRange }; // hibend and lobend
    CCMap<Range<uint8_t>> ccConditions { Default::ccRange };
    Range<uint8_t> keyswitchRange { Default::keyRange }; // sw_hikey and sw_lokey
    std::optional<uint8_t> keyswitch {}; // sw_last
    std::optional<uint8_t> keyswitchUp {}; // sw_up
    std::optional<uint8_t> keyswitchDown {}; // sw_down
    std::optional<uint8_t> previousNote {}; // sw_previous
    SfzVelocityOverride velocityOverride { Default::velocityOverride }; // sw_vel

    // Region logic: internal conditions
    Range<uint8_t> aftertouchRange { Default::aftertouchRange }; // hichanaft and lochanaft
    Range<float> bpmRange { Default::bpmRange }; // hibpm and lobpm
    Range<float> randRange { Default::randRange }; // hirand and lorand
    uint8_t sequenceLength { Default::sequenceLength }; // seq_length
    uint8_t sequencePosition { Default::sequencePosition }; // seq_position

    // Region logic: triggers
    SfzTrigger trigger { Default::trigger }; // trigger
    std::array<uint8_t, 128> lastNoteVelocities; // Keeps the velocities of the previous note-ons if the region has the trigger release_key
    CCMap<Range<uint8_t>> ccTriggers { Default::ccTriggerValueRange }; // on_loccN on_hiccN

    // Performance parameters: amplifier
    float volume { Default::volume }; // volume
    float amplitude { Default::amplitude }; // amplitude
    float pan { Default::pan }; // pan
    float width { Default::width }; // width
    float position { Default::position }; // position
    std::optional<CCValuePair> volumeCC; // volume_oncc
    std::optional<CCValuePair> amplitudeCC; // amplitude_oncc
    std::optional<CCValuePair> panCC; // pan_oncc
    std::optional<CCValuePair> widthCC; // width_oncc
    std::optional<CCValuePair> positionCC; // position_oncc
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

    // Performance parameters: pitch
    uint8_t pitchKeycenter { Default::pitchKeycenter }; // pitch_keycenter
    int pitchKeytrack { Default::pitchKeytrack }; // pitch_keytrack
    int pitchRandom { Default::pitchRandom }; // pitch_random
    int pitchVeltrack { Default::pitchVeltrack }; // pitch_veltrack
    int transpose { Default::transpose }; // transpose
    int tune { Default::tune }; // tune

    // Envelopes
    EGDescription amplitudeEG;
    EGDescription pitchEG;
    EGDescription filterEG;

    double sampleRate { config::defaultSampleRate };
    std::shared_ptr<AudioBuffer<float>> preloadedData { nullptr };
private:
    bool keySwitched { true };
    bool previousKeySwitched { true };
    bool sequenceSwitched { true };
    bool pitchSwitched { true };
    bool bpmSwitched { true };
    bool aftertouchSwitched { true };
    std::bitset<128> ccSwitched;

    int activeNotesInRange { -1 };
    int sequenceCounter { 0 };

    std::uniform_real_distribution<float> gainDistribution { -sfz::Default::ampRandom, sfz::Default::ampRandom };
    std::uniform_real_distribution<float> delayDistribution { 0, sfz::Default::delayRandom };
    std::uniform_int_distribution<uint32_t> offsetDistribution { 0, sfz::Default::offsetRandom };
    std::uniform_int_distribution<int> pitchDistribution { -sfz::Default::pitchRandom, sfz::Default::pitchRandom };
    LEAK_DETECTOR(Region);
};

} // namespace sfz