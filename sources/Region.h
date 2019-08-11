#pragma once
#include <optional>
#include <vector>
#include <string>
#include "Opcode.h"
#include "FilePool.h"
#include "EGDescription.h"
#include "Defaults.h"
#include "CCMap.h"
#include <bitset>

namespace sfz
{
struct Region
{
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
    bool registerNoteOn(int channel, int noteNumber, uint8_t velocity, float randValue);
    bool registerNoteOff(int channel, int noteNumber, uint8_t velocity, float randValue);
    bool registerCC(int channel, int ccNumber, uint8_t ccValue);
    void registerPitchWheel(int channel, int pitch);
    void registerAftertouch(int channel, uint8_t aftertouch);
    void registerTempo(float secondsPerQuarter);
    bool isStereo() const noexcept;
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
    Range<uint8_t> keyRange{ Default::keyRange }; //lokey, hikey and key
    Range<uint8_t> velocityRange{ Default::velocityRange }; // hivel and lovel

    // Region logic: MIDI conditions
    Range<uint8_t> channelRange{ Default::channelRange }; //lochan and hichan
    Range<int> bendRange{ Default::bendRange }; // hibend and lobend
    CCMap<Range<uint8_t>> ccConditions { Default::ccRange };
    Range<uint8_t> keyswitchRange{ Default::keyRange }; // sw_hikey and sw_lokey
    std::optional<uint8_t> keyswitch {}; // sw_last
    std::optional<uint8_t> keyswitchUp {}; // sw_up
    std::optional<uint8_t> keyswitchDown {}; // sw_down
    std::optional<uint8_t> previousNote {}; // sw_previous
    SfzVelocityOverride velocityOverride { Default::velocityOverride }; // sw_vel

    // Region logic: internal conditions
    Range<uint8_t> aftertouchRange{ Default::aftertouchRange }; // hichanaft and lochanaft
    Range<float> bpmRange{ Default::bpmRange }; // hibpm and lobpm
    Range<float> randRange{ Default::randRange }; // hirand and lorand
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

    // Performance parameters: pitch
    uint8_t pitchKeycenter{Default::pitchKeycenter}; // pitch_keycenter
    int pitchKeytrack{ Default::pitchKeytrack }; // pitch_keytrack
    int pitchRandom{ Default::pitchRandom }; // pitch_random
    int pitchVeltrack{ Default::pitchVeltrack }; // pitch_veltrack
    int transpose { Default::transpose }; // transpose
    int tune { Default::tune }; // tune

    // Envelopes
    EGDescription amplitudeEG;
    EGDescription pitchEG;
    EGDescription filterEG;

    double sampleRate { config::defaultSampleRate };
    int numChannels { 1 };
    std::shared_ptr<StereoBuffer<float>> preloadedData { nullptr };
private:
    bool keySwitched { true };
    bool previousKeySwitched { true };
    bool sequenceSwitched { true };
    bool pitchSwitched { true };
    bool bpmSwitched { true };
    bool aftertouchSwitched { true };
    std::bitset<128> ccSwitched;
    bool allCCSwitched { true };

    int activeNotesInRange { -1 };
    int sequenceCounter { 0 };
};
} // namespace sfz