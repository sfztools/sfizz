#include "Region.h"
#include "Helpers.h"
#include "absl/strings/str_replace.h"
#include <random>

bool sfz::Region::parseOpcode(const Opcode& opcode)
{
    switch (hash(opcode.opcode)) {
    // Sound source: sample playback
    case hash("sample"):
        sample = absl::StrReplaceAll(trim(opcode.value), { { "\\", "/" } });
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
        setRangeEndFromOpcode(opcode, keyRange, Default::keyRange);
        break;
    case hash("key"):
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
    case hash("lochan"):
        setRangeStartFromOpcode(opcode, channelRange, Default::channelRange);
        break;
    case hash("hichan"):
        setRangeEndFromOpcode(opcode, channelRange, Default::channelRange);
        break;
    case hash("lobend"):
        setRangeStartFromOpcode(opcode, bendRange, Default::bendRange);
        break;
    case hash("hibend"):
        setRangeEndFromOpcode(opcode, bendRange, Default::bendRange);
        break;
    case hash("locc"):
        if (opcode.parameter) {
            setRangeStartFromOpcode(opcode, ccConditions[*opcode.parameter], Default::ccRange);
        }
        break;
    case hash("hicc"):
        if (opcode.parameter)
            setRangeEndFromOpcode(opcode, ccConditions[*opcode.parameter], Default::ccRange);
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
            setRangeStartFromOpcode(opcode, ccTriggers[*opcode.parameter], Default::ccRange);
        break;
    case hash("on_hicc"):
        if (opcode.parameter)
            setRangeEndFromOpcode(opcode, ccTriggers[*opcode.parameter], Default::ccRange);
        break;

    // Performance parameters: amplifier
    case hash("volume"):
        setValueFromOpcode(opcode, volume, Default::volumeRange);
        break;
    // case hash("volume_oncc"):
    //     if (opcode.parameter)
    //         setValueFromOpcode(opcode, volumeCCTriggers[*opcode.parameter], Default::volumeCCRange);
    //     break;
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
        gainDistribution.param(std::uniform_real_distribution<float>::param_type(-ampRandom, ampRandom));
        break;
    case hash("amp_velcurve_"):
        if (opcode.parameter && Default::ccRange.containsWithEnd(*opcode.parameter)) {
            if (auto value = readOpcode(opcode.value, Default::ampVelcurveRange); value)
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
        break;
    case hash("transpose"):
        setValueFromOpcode(opcode, transpose, Default::transposeRange);
        break;
    case hash("tune"):
        setValueFromOpcode(opcode, tune, Default::tuneRange);
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
    return keySwitched && previousKeySwitched && sequenceSwitched && pitchSwitched && bpmSwitched && aftertouchSwitched && allCCSwitched;
}

bool sfz::Region::registerNoteOn(int channel, int noteNumber, uint8_t velocity, float randValue)
{
    const bool chanOk = channelRange.containsWithEnd(channel);
    if (!chanOk)
        return false;

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
        // Update the number of notes playing for the region
        activeNotesInRange++;

        // Sequence activation
        sequenceCounter += 1;
        if ((sequenceCounter % sequenceLength) == sequencePosition - 1)
            sequenceSwitched = true;
        else
            sequenceSwitched = false;

        // Velocity memory for release_key and for sw_vel=previous
        if (trigger == SfzTrigger::release_key || velocityOverride == SfzVelocityOverride::previous)
            lastNoteVelocities[noteNumber] = velocity;

        if (previousNote) {
            if (*previousNote == noteNumber)
                previousKeySwitched = true;
            else
                previousKeySwitched = false;
        }
    }

    if (!isSwitchedOn())
        return false;

    if (previousNote && !(previousKeySwitched && noteNumber != *previousNote))
        return false;

    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue) || (randValue == 1.0f && randRange.getEnd() == 1.0f);
    const bool firstLegatoNote = (trigger == SfzTrigger::first && activeNotesInRange == 0);
    const bool attackTrigger = (trigger == SfzTrigger::attack);
    const bool notFirstLegatoNote = (trigger == SfzTrigger::legato && activeNotesInRange > 0);

    return keyOk && velOk && chanOk && randOk && (attackTrigger || firstLegatoNote || notFirstLegatoNote);
}

bool sfz::Region::registerNoteOff(int channel, int noteNumber, uint8_t velocity [[maybe_unused]], float randValue)
{
    const bool chanOk = channelRange.containsWithEnd(channel);
    if (!chanOk)
        return false;

    if (keyswitchRange.containsWithEnd(noteNumber)) {
        if (keyswitchDown && *keyswitchDown == noteNumber)
            keySwitched = false;

        if (keyswitchUp && *keyswitchUp == noteNumber)
            keySwitched = true;
    }

    const bool keyOk = keyRange.containsWithEnd(noteNumber);

    // Update the number of notes playing for the region
    if (keyOk)
        activeNotesInRange--;

    if (!isSwitchedOn())
        return false;

    const bool randOk = randRange.contains(randValue);
    const bool releaseTrigger = (trigger == SfzTrigger::release || trigger == SfzTrigger::release_key);
    return keyOk && chanOk && randOk && releaseTrigger;
}

bool sfz::Region::registerCC(int channel, int ccNumber, uint8_t ccValue)
{
    if (!channelRange.containsWithEnd(channel))
        return false;

    if (ccConditions.getWithDefault(ccNumber).containsWithEnd(ccValue))
        ccSwitched.set(ccNumber, true);
    else
        ccSwitched.set(ccNumber, false);

    if (ccSwitched.all())
        allCCSwitched = true;
    else
        allCCSwitched = false;

    if (ccTriggers.contains(ccNumber) && ccTriggers.at(ccNumber).containsWithEnd(ccValue))
        return true;
    else
        return false;
}

void sfz::Region::registerPitchWheel(int channel, int pitch)
{
    if (!channelRange.containsWithEnd(channel))
        return;

    if (bendRange.containsWithEnd(pitch))
        pitchSwitched = true;
    else
        pitchSwitched = false;
}

void sfz::Region::registerAftertouch(int channel, uint8_t aftertouch)
{
    if (!channelRange.containsWithEnd(channel))
        return;

    if (aftertouchRange.containsWithEnd(aftertouch))
        aftertouchSwitched = true;
    else
        aftertouchSwitched = false;
}

void sfz::Region::registerTempo(float secondsPerQuarter)
{
    const float bpm = 60.0f / secondsPerQuarter;
    if (bpmRange.containsWithEnd(bpm))
        bpmSwitched = true;
    else
        bpmSwitched = false;
}