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

#include "Region.h"
#include "Defaults.h"
#include "MathHelpers.h"
#include "Debug.h"
#include "Opcode.h"
#include "StringViewHelpers.h"
#include "MidiState.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_cat.h"
#include <random>

bool sfz::Region::parseOpcode(const Opcode& opcode)
{
    // Check that the parameter is well formed
    if (opcode.parameter && !sfz::Default::ccNumberRange.containsWithEnd(*opcode.parameter)) {
        DBG("Wrong parameter value (" << std::to_string(*opcode.parameter) << ") for opcode " << opcode.opcode);
        return false;
    }

    switch (hash(opcode.opcode)) {
    // Sound source: sample playback
    case hash("sample"):
        {
            const auto trimmedSample = trim(opcode.value);
            if (trimmedSample.empty())
                break;

            if (trimmedSample[0] == '*')
                sample = std::string(trimmedSample);
            else
                sample = absl::StrCat(defaultPath, absl::StrReplaceAll(trimmedSample, { { "\\", "/" } }));
        }
        break;
    case hash("delay"):
        setValueFromOpcode(opcode, delay, Default::delayRange);
        break;
    case hash("delay_random"):
        setValueFromOpcode(opcode, delayRandom, Default::delayRange);
        delayDistribution.param(std::uniform_real_distribution<float>::param_type(0, delayRandom));
        break;
    case hash("offset"):
        setValueFromOpcode(opcode, offset, Default::offsetRange);
        break;
    case hash("offset_random"):
        setValueFromOpcode(opcode, offsetRandom, Default::offsetRange);
        offsetDistribution.param(std::uniform_int_distribution<uint32_t>::param_type(0, offsetRandom));
        break;
    case hash("end"):
        setValueFromOpcode(opcode, sampleEnd, Default::sampleEndRange);
        break;
    case hash("count"):
        setValueFromOpcode(opcode, sampleCount, Default::sampleCountRange);
        break;
    case hash("loopmode"):
    case hash("loop_mode"):
        switch (hash(opcode.value)) {
        case hash("no_loop"):
            loopMode = SfzLoopMode::no_loop;
            break;
        case hash("one_shot"):
            loopMode = SfzLoopMode::one_shot;
            break;
        case hash("loop_continuous"):
            loopMode = SfzLoopMode::loop_continuous;
            break;
        case hash("loop_sustain"):
            loopMode = SfzLoopMode::loop_sustain;
            break;
        default:
            DBG("Unkown loop mode:" << std::string(opcode.value));
        }
        break;
    case hash("loopend"):
    case hash("loop_end"):
        setRangeEndFromOpcode(opcode, loopRange, Default::loopRange);
        break;
    case hash("loopstart"):
    case hash("loop_start"):
        setRangeStartFromOpcode(opcode, loopRange, Default::loopRange);
        break;

    // Instrument settings: voice lifecycle
    case hash("group"):
        setValueFromOpcode(opcode, group, Default::groupRange);
        break;
    case hash("offby"):
    case hash("off_by"):
        setValueFromOpcode(opcode, offBy, Default::groupRange);
        break;
    case hash("off_mode"):
        switch (hash(opcode.value)) {
        case hash("fast"):
            offMode = SfzOffMode::fast;
            break;
        case hash("normal"):
            offMode = SfzOffMode::normal;
            break;
        default:
            DBG("Unkown off mode:" << std::string(opcode.value));
        }
        break;
    // Region logic: key mapping
    case hash("lokey"):
        setRangeStartFromOpcode(opcode, keyRange, Default::keyRange);
        break;
    case hash("hikey"):
        triggerOnCC = (opcode.value == "-1");
        setRangeEndFromOpcode(opcode, keyRange, Default::keyRange);
        break;
    case hash("key"):
        triggerOnCC = (opcode.value == "-1");
        setRangeStartFromOpcode(opcode, keyRange, Default::keyRange);
        setRangeEndFromOpcode(opcode, keyRange, Default::keyRange);
        setValueFromOpcode(opcode, pitchKeycenter, Default::keyRange);
        break;
    case hash("lovel"):
        setRangeStartFromOpcode(opcode, velocityRange, Default::velocityRange);
        break;
    case hash("hivel"):
        setRangeEndFromOpcode(opcode, velocityRange, Default::velocityRange);
        break;

    // Region logic: MIDI conditions
    case hash("lobend"):
        setRangeStartFromOpcode(opcode, bendRange, Default::bendRange);
        break;
    case hash("hibend"):
        setRangeEndFromOpcode(opcode, bendRange, Default::bendRange);
        break;
    case hash("locc"):
        if (opcode.parameter) {
            setRangeStartFromOpcode(opcode, ccConditions[*opcode.parameter], Default::ccValueRange);
        }
        break;
    case hash("hicc"):
        if (opcode.parameter)
            setRangeEndFromOpcode(opcode, ccConditions[*opcode.parameter], Default::ccValueRange);
        break;
    case hash("sw_lokey"):
        setRangeStartFromOpcode(opcode, keyswitchRange, Default::keyRange);
        break;
    case hash("sw_hikey"):
        setRangeEndFromOpcode(opcode, keyswitchRange, Default::keyRange);
        break;
    case hash("sw_last"):
        setValueFromOpcode(opcode, keyswitch, Default::keyRange);
        keySwitched = false;
        break;
    case hash("sw_down"):
        setValueFromOpcode(opcode, keyswitchDown, Default::keyRange);
        keySwitched = false;
        break;
    case hash("sw_up"):
        setValueFromOpcode(opcode, keyswitchUp, Default::keyRange);
        break;
    case hash("sw_previous"):
        setValueFromOpcode(opcode, previousNote, Default::keyRange);
        previousKeySwitched = false;
        break;
    case hash("sw_vel"):
        switch (hash(opcode.value)) {
        case hash("current"):
            velocityOverride = SfzVelocityOverride::current;
            break;
        case hash("previous"):
            velocityOverride = SfzVelocityOverride::previous;
            break;
        default:
            DBG("Unknown velocity mode: " << std::string(opcode.value));
        }
        break;

    case hash("sustain_sw"):
        checkSustain = readBooleanFromOpcode(opcode).value_or(Default::checkSustain);
        break;
    case hash("sostenuto_sw"):
        checkSostenuto = readBooleanFromOpcode(opcode).value_or(Default::checkSostenuto);
        break;
    // Region logic: internal conditions
    case hash("lochanaft"):
        setRangeStartFromOpcode(opcode, aftertouchRange, Default::aftertouchRange);
        break;
    case hash("hichanaft"):
        setRangeEndFromOpcode(opcode, aftertouchRange, Default::aftertouchRange);
        break;
    case hash("lobpm"):
        setRangeStartFromOpcode(opcode, bpmRange, Default::bpmRange);
        break;
    case hash("hibpm"):
        setRangeEndFromOpcode(opcode, bpmRange, Default::bpmRange);
        break;
    case hash("lorand"):
        setRangeStartFromOpcode(opcode, randRange, Default::randRange);
        break;
    case hash("hirand"):
        setRangeEndFromOpcode(opcode, randRange, Default::randRange);
        break;
    case hash("seq_length"):
        setValueFromOpcode(opcode, sequenceLength, Default::sequenceRange);
        break;
    case hash("seq_position"):
        setValueFromOpcode(opcode, sequencePosition, Default::sequenceRange);
        sequenceSwitched = (opcode.value == "1");
        break;
    // Region logic: triggers
    case hash("trigger"):
        switch (hash(opcode.value)) {
        case hash("attack"):
            trigger = SfzTrigger::attack;
            break;
        case hash("first"):
            trigger = SfzTrigger::first;
            break;
        case hash("legato"):
            trigger = SfzTrigger::legato;
            break;
        case hash("release"):
            trigger = SfzTrigger::release;
            break;
        case hash("release_key"):
            trigger = SfzTrigger::release_key;
            break;
        default:
            DBG("Unknown trigger mode: " << std::string(opcode.value));
        }
        break;
    case hash("on_locc"):
        if (opcode.parameter)
            setRangeStartFromOpcode(opcode, ccTriggers[*opcode.parameter], Default::ccTriggerValueRange);
        break;
    case hash("on_hicc"):
        if (opcode.parameter)
            setRangeEndFromOpcode(opcode, ccTriggers[*opcode.parameter], Default::ccTriggerValueRange);
        break;

    // Performance parameters: amplifier
    case hash("volume"):
        setValueFromOpcode(opcode, volume, Default::volumeRange);
        break;
    case hash("gain_cc"):
    case hash("gain_oncc"):
    case hash("volume_oncc"):
        setCCPairFromOpcode(opcode, volumeCC, Default::volumeCCRange);
        break;
    case hash("amplitude"):
        setValueFromOpcode(opcode, amplitude, Default::amplitudeRange);
        break;
    case hash("amplitude_cc"):
    case hash("amplitude_oncc"):
        setCCPairFromOpcode(opcode, amplitudeCC, Default::amplitudeRange);
        break;
    case hash("pan"):
        setValueFromOpcode(opcode, pan, Default::panRange);
        break;
    case hash("pan_oncc"):
        setCCPairFromOpcode(opcode, panCC, Default::panCCRange);
        break;
    case hash("position"):
        setValueFromOpcode(opcode, position, Default::positionRange);
        break;
    case hash("position_oncc"):
        setCCPairFromOpcode(opcode, positionCC, Default::positionCCRange);
        break;
    case hash("width"):
        setValueFromOpcode(opcode, width, Default::widthRange);
        break;
    case hash("width_oncc"):
        setCCPairFromOpcode(opcode, widthCC, Default::widthCCRange);
        break;
    case hash("amp_keycenter"):
        setValueFromOpcode(opcode, ampKeycenter, Default::keyRange);
        break;
    case hash("amp_keytrack"):
        setValueFromOpcode(opcode, ampKeytrack, Default::ampKeytrackRange);
        break;
    case hash("amp_veltrack"):
        setValueFromOpcode(opcode, ampVeltrack, Default::ampVeltrackRange);
        break;
    case hash("amp_random"):
        setValueFromOpcode(opcode, ampRandom, Default::ampRandomRange);
        volumeDistribution.param(std::uniform_real_distribution<float>::param_type(0, ampRandom));
        break;
    case hash("amp_velcurve_"):
        {
            auto value = readOpcode(opcode.value, Default::ampVelcurveRange);
            if (value)
                velocityPoints.emplace_back(*opcode.parameter, *value);
        }
        break;
    case hash("xfin_lokey"):
        setRangeStartFromOpcode(opcode, crossfadeKeyInRange, Default::keyRange);
        break;
    case hash("xfin_hikey"):
        setRangeEndFromOpcode(opcode, crossfadeKeyInRange, Default::keyRange);
        break;
    case hash("xfout_lokey"):
        setRangeStartFromOpcode(opcode, crossfadeKeyOutRange, Default::keyRange);
        break;
    case hash("xfout_hikey"):
        setRangeEndFromOpcode(opcode, crossfadeKeyOutRange, Default::keyRange);
        break;
    case hash("xfin_lovel"):
        setRangeStartFromOpcode(opcode, crossfadeVelInRange, Default::velocityRange);
        break;
    case hash("xfin_hivel"):
        setRangeEndFromOpcode(opcode, crossfadeVelInRange, Default::velocityRange);
        break;
    case hash("xfout_lovel"):
        setRangeStartFromOpcode(opcode, crossfadeVelOutRange, Default::velocityRange);
        break;
    case hash("xfout_hivel"):
        setRangeEndFromOpcode(opcode, crossfadeVelOutRange, Default::velocityRange);
        break;
    case hash("xf_keycurve"):
        switch (hash(opcode.value)) {
        case hash("power"):
            crossfadeKeyCurve = SfzCrossfadeCurve::power;
            break;
        case hash("gain"):
            crossfadeKeyCurve = SfzCrossfadeCurve::gain;
            break;
        default:
            DBG("Unknown crossfade power curve: " << std::string(opcode.value));
        }
        break;
    case hash("xf_velcurve"):
        switch (hash(opcode.value)) {
        case hash("power"):
            crossfadeVelCurve = SfzCrossfadeCurve::power;
            break;
        case hash("gain"):
            crossfadeVelCurve = SfzCrossfadeCurve::gain;
            break;
        default:
            DBG("Unknown crossfade power curve: " << std::string(opcode.value));
        }
        break;
    case hash("xfin_locc"):
        if (opcode.parameter) {
            setRangeStartFromOpcode(opcode, crossfadeCCInRange[*opcode.parameter], Default::ccValueRange);
        }
        break;
    case hash("xfin_hicc"):
        if (opcode.parameter) {
            setRangeEndFromOpcode(opcode, crossfadeCCInRange[*opcode.parameter], Default::ccValueRange);
        }
        break;
    case hash("xfout_locc"):
        if (opcode.parameter) {
            setRangeStartFromOpcode(opcode, crossfadeCCOutRange[*opcode.parameter], Default::ccValueRange);
        }
        break;
    case hash("xfout_hicc"):
        if (opcode.parameter) {
            setRangeEndFromOpcode(opcode, crossfadeCCOutRange[*opcode.parameter], Default::ccValueRange);
        }
        break;
    case hash("xf_cccurve"):
        switch (hash(opcode.value)) {
        case hash("power"):
            crossfadeCCCurve = SfzCrossfadeCurve::power;
            break;
        case hash("gain"):
            crossfadeCCCurve = SfzCrossfadeCurve::gain;
            break;
        default:
            DBG("Unknown crossfade power curve: " << std::string(opcode.value));
        }
        break;
    case hash("rt_decay"):
        setValueFromOpcode(opcode, rtDecay, Default::rtDecayRange);
        break;

    // Performance parameters: pitch
    case hash("pitch_keycenter"):
        setValueFromOpcode(opcode, pitchKeycenter, Default::keyRange);
        break;
    case hash("pitch_keytrack"):
        setValueFromOpcode(opcode, pitchKeytrack, Default::pitchKeytrackRange);
        break;
    case hash("pitch_veltrack"):
        setValueFromOpcode(opcode, pitchVeltrack, Default::pitchVeltrackRange);
        break;
    case hash("pitch_random"):
        setValueFromOpcode(opcode, pitchRandom, Default::pitchRandomRange);
        pitchDistribution.param(std::uniform_int_distribution<int>::param_type(-pitchRandom, pitchRandom));
        break;
    case hash("transpose"):
        setValueFromOpcode(opcode, transpose, Default::transposeRange);
        break;
    case hash("tune"):
        setValueFromOpcode(opcode, tune, Default::tuneRange);
        break;
    case hash("bend_up"):
        setValueFromOpcode(opcode, bendUp, Default::bendBoundRange);
        break;
    case hash("bend_down"):
        setValueFromOpcode(opcode, bendDown, Default::bendBoundRange);
        break;
    case hash("bend_step"):
        setValueFromOpcode(opcode, bendStep, Default::bendStepRange);
        break;

    // Amplitude Envelope
    case hash("ampeg_attack"):
        setValueFromOpcode(opcode, amplitudeEG.attack, Default::egTimeRange);
        break;
    case hash("ampeg_decay"):
        setValueFromOpcode(opcode, amplitudeEG.decay, Default::egTimeRange);
        break;
    case hash("ampeg_delay"):
        setValueFromOpcode(opcode, amplitudeEG.delay, Default::egTimeRange);
        break;
    case hash("ampeg_hold"):
        setValueFromOpcode(opcode, amplitudeEG.hold, Default::egTimeRange);
        break;
    case hash("ampeg_release"):
        setValueFromOpcode(opcode, amplitudeEG.release, Default::egTimeRange);
        break;
    case hash("ampeg_start"):
        setValueFromOpcode(opcode, amplitudeEG.start, Default::egPercentRange);
        break;
    case hash("ampeg_sustain"):
        setValueFromOpcode(opcode, amplitudeEG.sustain, Default::egPercentRange);
        break;
    case hash("ampeg_vel2attack"):
        setValueFromOpcode(opcode, amplitudeEG.vel2attack, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_vel2decay"):
        setValueFromOpcode(opcode, amplitudeEG.vel2decay, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_vel2delay"):
        setValueFromOpcode(opcode, amplitudeEG.vel2delay, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_vel2hold"):
        setValueFromOpcode(opcode, amplitudeEG.vel2hold, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_vel2release"):
        setValueFromOpcode(opcode, amplitudeEG.vel2release, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_vel2sustain"):
        setValueFromOpcode(opcode, amplitudeEG.vel2sustain, Default::egOnCCPercentRange);
        break;
    case hash("ampeg_attack_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccAttack, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_decay_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccDecay, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_delay_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccDelay, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_hold_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccHold, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_release_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccRelease, Default::egOnCCTimeRange);
        break;
    case hash("ampeg_start_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccStart, Default::egOnCCPercentRange);
        break;
    case hash("ampeg_sustain_oncc"):
        setCCPairFromOpcode(opcode, amplitudeEG.ccSustain, Default::egOnCCPercentRange);
        break;

    // Ignored opcodes
    case hash("hichan"):
    case hash("lochan"):
    case hash("ampeg_depth"):
    case hash("ampeg_vel2depth"):
        break;
    default:
        return false;
    }

    return true;
}

bool sfz::Region::isSwitchedOn() const noexcept
{
    return keySwitched && previousKeySwitched && sequenceSwitched && pitchSwitched && bpmSwitched && aftertouchSwitched && ccSwitched.all();
}

bool sfz::Region::registerNoteOn(int noteNumber, uint8_t velocity, float randValue) noexcept
{
    if (keyswitchRange.containsWithEnd(noteNumber)) {
        if (keyswitch) {
            if (*keyswitch == noteNumber)
                keySwitched = true;
            else
                keySwitched = false;
        }

        if (keyswitchDown && *keyswitchDown == noteNumber)
            keySwitched = true;

        if (keyswitchUp && *keyswitchUp == noteNumber)
            keySwitched = false;
    }

    const bool keyOk = keyRange.containsWithEnd(noteNumber);
    if (keyOk) {
        // Sequence activation
        sequenceCounter += 1;
        if ((sequenceCounter % sequenceLength) == sequencePosition - 1)
            sequenceSwitched = true;
        else
            sequenceSwitched = false;

        if (previousNote) {
            if (*previousNote == noteNumber)
                previousKeySwitched = true;
            else
                previousKeySwitched = false;
        }
    }

    if (!isSwitchedOn())
        return false;

    if (triggerOnCC)
        return false;

    if (previousNote && !(previousKeySwitched && noteNumber != *previousNote))
        return false;

    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue) || (randValue == 1.0f && randRange.getEnd() == 1.0f);
    const bool firstLegatoNote = (trigger == SfzTrigger::first && midiState.getActiveNotes() == 1);
    const bool attackTrigger = (trigger == SfzTrigger::attack);
    const bool notFirstLegatoNote = (trigger == SfzTrigger::legato && midiState.getActiveNotes() > 1);

    return keyOk && velOk && randOk && (attackTrigger || firstLegatoNote || notFirstLegatoNote);
}

bool sfz::Region::registerNoteOff(int noteNumber, uint8_t velocity [[maybe_unused]], float randValue) noexcept
{
    if (keyswitchRange.containsWithEnd(noteNumber)) {
        if (keyswitchDown && *keyswitchDown == noteNumber)
            keySwitched = false;

        if (keyswitchUp && *keyswitchUp == noteNumber)
            keySwitched = true;
    }

    const bool keyOk = keyRange.containsWithEnd(noteNumber);

    if (!isSwitchedOn())
        return false;

    if (triggerOnCC)
        return false;

    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue);
    const bool releaseTrigger = (trigger == SfzTrigger::release || trigger == SfzTrigger::release_key);
    return keyOk && velOk && randOk && releaseTrigger;
}

bool sfz::Region::registerCC(int ccNumber, uint8_t ccValue) noexcept
{
    if (ccConditions.getWithDefault(ccNumber).containsWithEnd(ccValue))
        ccSwitched.set(ccNumber, true);
    else
        ccSwitched.set(ccNumber, false);

    if (!isSwitchedOn())
        return false;

    if (!triggerOnCC)
        return false;

    if (ccTriggers.contains(ccNumber) && ccTriggers.at(ccNumber).containsWithEnd(ccValue))
        return true;
    else
        return false;
}

void sfz::Region::registerPitchWheel(int pitch) noexcept
{
    if (bendRange.containsWithEnd(pitch))
        pitchSwitched = true;
    else
        pitchSwitched = false;
}

void sfz::Region::registerAftertouch(uint8_t aftertouch) noexcept
{
    if (aftertouchRange.containsWithEnd(aftertouch))
        aftertouchSwitched = true;
    else
        aftertouchSwitched = false;
}

void sfz::Region::registerTempo(float secondsPerQuarter) noexcept
{
    const float bpm = 60.0f / secondsPerQuarter;
    if (bpmRange.containsWithEnd(bpm))
        bpmSwitched = true;
    else
        bpmSwitched = false;
}

float sfz::Region::getBasePitchVariation(int noteNumber, uint8_t velocity) noexcept
{
    auto pitchVariationInCents = pitchKeytrack * (noteNumber - (int)pitchKeycenter); // note difference with pitch center
    pitchVariationInCents += tune; // sample tuning
    pitchVariationInCents += config::centPerSemitone * transpose; // sample transpose
    pitchVariationInCents += velocity / 127 * pitchVeltrack; // track velocity
    pitchVariationInCents += pitchDistribution(Random::randomGenerator); // random pitch changes
    return centsFactor(pitchVariationInCents);
}

float sfz::Region::getBaseVolumedB(int noteNumber) noexcept
{
    auto baseVolumedB = volume + volumeDistribution(Random::randomGenerator);
    if (trigger == SfzTrigger::release || trigger == SfzTrigger::release_key)
        baseVolumedB -= rtDecay * midiState.getNoteDuration(noteNumber);
    return baseVolumedB;
}

float sfz::Region::getBaseGain() noexcept
{
    return normalizePercents(amplitude);
}

uint32_t sfz::Region::getOffset(Oversampling factor) noexcept
{
    return (offset + offsetDistribution(Random::randomGenerator)) * static_cast<uint32_t>(factor);
}

float sfz::Region::getDelay() noexcept
{
    return delay + delayDistribution(Random::randomGenerator);
}

uint32_t sfz::Region::trueSampleEnd(Oversampling factor) const noexcept
{
    return min(sampleEnd, loopRange.getEnd()) * static_cast<uint32_t>(factor);
}

uint32_t sfz::Region::loopStart(Oversampling factor) const noexcept
{
    return loopRange.getStart() * static_cast<uint32_t>(factor);
}

uint32_t sfz::Region::loopEnd(Oversampling factor) const noexcept
{
    return loopRange.getEnd() * static_cast<uint32_t>(factor);
}

template<class T, class U>
float crossfadeIn(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value < crossfadeRange.getStart())
        return 0.0f;
    else if (value < crossfadeRange.getEnd()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / std::max(static_cast<float>(crossfadeRange.length()), 1.0f);
        if (curve == SfzCrossfadeCurve::power)
            return sqrt(crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return crossfadePosition;
    }

    return 1.0f;
}

template<class T, class U>
float crossfadeOut(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value > crossfadeRange.getEnd())
        return 0.0f;
    else if (value > crossfadeRange.getStart()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / std::max(static_cast<float>(crossfadeRange.length()), 1.0f);
        if (curve == SfzCrossfadeCurve::power)
            return std::sqrt(1 - crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return 1 - crossfadePosition;
    }

    return 1.0f;
}

float sfz::Region::getNoteGain(int noteNumber, uint8_t velocity) noexcept
{
    float baseGain { 1.0f };

    // Amplitude key tracking
    baseGain *= db2mag(ampKeytrack * static_cast<float>(noteNumber - ampKeycenter));

    // Crossfades related to the note number
    baseGain *= crossfadeIn(crossfadeKeyInRange, noteNumber, crossfadeKeyCurve);
    baseGain *= crossfadeOut(crossfadeKeyOutRange, noteNumber, crossfadeKeyCurve);

    // Amplitude velocity tracking
    baseGain *= velocityCurve(velocity);

    // Crossfades related to velocity
    baseGain *= crossfadeIn(crossfadeVelInRange, velocity, crossfadeVelCurve);
    baseGain *= crossfadeOut(crossfadeVelOutRange, velocity, crossfadeVelCurve);

    return baseGain;
}

float sfz::Region::getCrossfadeGain(const sfz::SfzCCArray& ccState) noexcept
{
    float gain { 1.0f };

    // Crossfades due to CC states
    for (const auto& valuePair : crossfadeCCInRange) {
        const auto ccValue = ccState[valuePair.first];
        const auto crossfadeRange = valuePair.second;
        gain *= crossfadeIn(crossfadeRange, ccValue, crossfadeCCCurve);
    }

    for (const auto& valuePair : crossfadeCCOutRange) {
        const auto ccValue = ccState[valuePair.first];
        const auto crossfadeRange = valuePair.second;
        gain *= crossfadeOut(crossfadeRange, ccValue, crossfadeCCCurve);
    }

    return gain;
}

float sfz::Region::velocityCurve(uint8_t velocity) const noexcept
{
    float gain { 1.0f };

    if (velocityPoints.size() > 0) { // Custom velocity curve
        auto after = std::find_if(velocityPoints.begin(), velocityPoints.end(), [velocity](auto& val) { return val.first >= velocity; });
        auto before = after == velocityPoints.begin() ? velocityPoints.begin() : after - 1;
        // Linear interpolation
        float relativePositionInSegment {
            static_cast<float>(velocity - before->first) / static_cast<float>(after->first - before->first)
        };
        float segmentEndpoints { after->second - before->second };
        gain *= relativePositionInSegment * segmentEndpoints;
    } else { // Standard velocity curve
        const float floatVelocity { static_cast<float>(velocity) / 127.0f };
        // FIXME: Maybe there's a prettier way to check the boundaries?
        const float gaindB = [&]() {
            if (ampVeltrack >= 0)
                return floatVelocity == 0.0f ? -90.0f : 40 * std::log(floatVelocity) / std::log(10.0f);
            else
                return floatVelocity == 1.0f ? -90.0f : 40 * std::log(1 - floatVelocity) / std::log(10.0f);
        }();
        gain *= db2mag( gaindB * std::abs(ampVeltrack) / sfz::Default::ampVeltrackRange.getEnd());
    }

    return gain;
}

constexpr uint8_t offsetAndClamp(uint8_t key, int offset, sfz::Range<uint8_t> range)
{
    const int offsetKey { key + offset };
    if (offsetKey > std::numeric_limits<uint8_t>::max())
        return range.getEnd();
    if (offsetKey < std::numeric_limits<uint8_t>::min())
        return range.getStart();

    return range.clamp(static_cast<uint8_t>(offsetKey));
}

void sfz::Region::offsetAllKeys(int offset) noexcept
{
    // Offset key range
    if (keyRange != Default::keyRange) {
        const auto start = keyRange.getStart();
        const auto end = keyRange.getEnd();
        keyRange.setStart(offsetAndClamp(start, offset, Default::keyRange));
        keyRange.setEnd(offsetAndClamp(end, offset, Default::keyRange));
    }
    pitchKeycenter = offsetAndClamp(pitchKeycenter, offset, Default::keyRange);

    // Offset key switches
    if (keyswitchRange != Default::keyRange) {
        const auto start = keyswitchRange.getStart();
        const auto end = keyswitchRange.getEnd();
        keyswitchRange.setStart(offsetAndClamp(start, offset, Default::keyRange));
        keyswitchRange.setEnd(offsetAndClamp(end, offset, Default::keyRange));
    }
    if (keyswitchUp)
        keyswitchUp = offsetAndClamp(*keyswitchUp, offset, Default::keyRange);
    if (keyswitch)
        keyswitch = offsetAndClamp(*keyswitch, offset, Default::keyRange);
    if (keyswitchDown)
        keyswitchDown = offsetAndClamp(*keyswitchDown, offset, Default::keyRange);
    if (previousNote)
        previousNote = offsetAndClamp(*previousNote, offset, Default::keyRange);

    // Offset crossfade ranges
    if (crossfadeKeyInRange != Default::crossfadeKeyInRange) {
        const auto start = crossfadeKeyInRange.getStart();
        const auto end = crossfadeKeyInRange.getEnd();
        crossfadeKeyInRange.setStart(offsetAndClamp(start, offset, Default::keyRange));
        crossfadeKeyInRange.setEnd(offsetAndClamp(end, offset, Default::keyRange));
    }

    if (crossfadeKeyOutRange != Default::crossfadeKeyOutRange) {
        const auto start = crossfadeKeyOutRange.getStart();
        const auto end = crossfadeKeyOutRange.getEnd();
        crossfadeKeyOutRange.setStart(offsetAndClamp(start, offset, Default::keyRange));
        crossfadeKeyOutRange.setEnd(offsetAndClamp(end, offset, Default::keyRange));
    }
}
