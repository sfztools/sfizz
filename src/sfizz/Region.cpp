// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Region.h"
#include "MathHelpers.h"
#include "Macros.h"
#include "Debug.h"
#include "Opcode.h"
#include "StringViewHelpers.h"
#include "ModifierHelpers.h"
#include "modulations/ModId.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_cat.h"
#include "absl/algorithm/container.h"
#include <random>
#include <cassert>

template<class T>
bool extendIfNecessary(std::vector<T>& vec, unsigned size, unsigned defaultCapacity)
{
    if (size == 0)
        return false;

    if (vec.capacity() == 0)
        vec.reserve(defaultCapacity);

    if (vec.size() < size)
        vec.resize(size);

    return true;
}

bool sfz::Region::parseOpcode(const Opcode& rawOpcode)
{
    const Opcode opcode = rawOpcode.cleanUp(kOpcodeScopeRegion);

    switch (opcode.lettersOnlyHash) {
    // Helper for ccN processing
    #define case_any_ccN(x)        \
        case hash(x "_oncc&"):     \
        case hash(x "_curvecc&"):  \
        case hash(x "_stepcc&"):   \
        case hash(x "_smoothcc&")

    #define LFO_EG_filter_EQ_target(sourceKey, targetKey, range)                                        \
        {                                                                                               \
            const auto number = opcode.parameters.front();                                              \
            if (number == 0)                                                                            \
                return false;                                                                           \
                                                                                                        \
            const auto index = opcode.parameters.size() == 2 ? opcode.parameters.back() - 1 : 0;        \
            if (!extendIfNecessary(filters, index + 1, Default::numFilters))                            \
                return false;                                                                           \
                                                                                                        \
            if (auto value = readOpcode(opcode.value, range)) {                                         \
                const ModKey source = ModKey::createNXYZ(sourceKey, id, number - 1);                    \
                const ModKey target = ModKey::createNXYZ(targetKey, id, index);                         \
                getOrCreateConnection(source, target).sourceDepth = *value;                             \
            }                                                                                           \
        }

    // Sound source: sample playback
    case hash("sample"):
        {
            const auto trimmedSample = trim(opcode.value);
            if (trimmedSample.empty())
                break;

            std::string filename;
            if (trimmedSample[0] == '*')
                filename = std::string(trimmedSample);
            else
                filename = absl::StrCat(defaultPath, absl::StrReplaceAll(trimmedSample, { { "\\", "/" } }));

            *sampleId = FileId(std::move(filename), sampleId->isReverse());
        }
        break;
    case hash("sample_quality"):
        {
            if (opcode.value == "-1")
                sampleQuality.reset();
            else
                setValueFromOpcode(opcode, sampleQuality, Default::sampleQualityRange);
            break;
        }
        break;
    case hash("direction"):
        *sampleId = sampleId->reversed(opcode.value == "reverse");
        break;
    case hash("delay"):
        setValueFromOpcode(opcode, delay, Default::delayRange);
        break;
    case hash("delay_random"):
        setValueFromOpcode(opcode, delayRandom, Default::delayRange);
        break;
    case hash("offset"):
        setValueFromOpcode(opcode, offset, Default::offsetRange);
        break;
    case hash("offset_random"):
        setValueFromOpcode(opcode, offsetRandom, Default::offsetRange);
        break;
    case hash("offset_oncc&"): // also offset_cc&
        if (opcode.parameters.back() > config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::offsetCCRange))
            offsetCC[opcode.parameters.back()] = *value;
        break;
    case hash("end"):
        setValueFromOpcode(opcode, sampleEnd, Default::sampleEndRange);
        break;
    case hash("count"):
        setValueFromOpcode(opcode, sampleCount, Default::sampleCountRange);
        break;
    case hash("loop_mode"): // also loopmode
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
            DBG("Unkown loop mode:" << opcode.value);
        }
        break;
    case hash("loop_end"): // also loopend
        setRangeEndFromOpcode(opcode, loopRange, Default::loopRange);
        break;
    case hash("loop_start"): // also loopstart
        setRangeStartFromOpcode(opcode, loopRange, Default::loopRange);
        break;
    case hash("loop_crossfade"):
        setValueFromOpcode(opcode, loopCrossfade, Default::loopCrossfadeRange);
        break;

    // Wavetable oscillator
    case hash("oscillator_phase"):
        if (auto value = readOpcode(opcode.value, Default::oscillatorPhaseRange))
            oscillatorPhase = (*value >= 0) ? wrapPhase(*value) : -1.0f;
        break;
    case hash("oscillator"):
        if (auto value = readBooleanFromOpcode(opcode))
            oscillatorEnabled = *value ? OscillatorEnabled::On : OscillatorEnabled::Off;
        break;
    case hash("oscillator_mode"):
        setValueFromOpcode(opcode, oscillatorMode, Default::oscillatorModeRange);
        break;
    case hash("oscillator_multi"):
        setValueFromOpcode(opcode, oscillatorMulti, Default::oscillatorMultiRange);
        break;
    case hash("oscillator_detune"):
        setValueFromOpcode(opcode, oscillatorDetune, Default::oscillatorDetuneRange);
        break;
    case_any_ccN("oscillator_detune"):
        processGenericCc(opcode, Default::oscillatorDetuneCCRange, ModKey::createNXYZ(ModId::OscillatorDetune, id));
        break;
    case hash("oscillator_mod_depth"):
        if (auto value = readOpcode(opcode.value, Default::oscillatorModDepthRange))
            oscillatorModDepth = normalizePercents(*value);
        break;
    case_any_ccN("oscillator_mod_depth"):
        processGenericCc(opcode, Default::oscillatorModDepthCCRange, ModKey::createNXYZ(ModId::OscillatorModDepth, id));
        break;
    case hash("oscillator_quality"):
        if (opcode.value == "-1")
            oscillatorQuality.reset();
        else
            setValueFromOpcode(opcode, oscillatorQuality, Default::oscillatorQualityRange);
        break;

    // Instrument settings: voice lifecycle
    case hash("group"): // also polyphony_group
        setValueFromOpcode(opcode, group, Default::groupRange);
        break;
    case hash("off_by"): // also offby
        if (opcode.value == "-1")
            offBy.reset();
        else
            setValueFromOpcode(opcode, offBy, Default::groupRange);
        break;
    case hash("off_mode"): // also offmode
        switch (hash(opcode.value)) {
        case hash("fast"):
            offMode = SfzOffMode::fast;
            break;
        case hash("normal"):
            offMode = SfzOffMode::normal;
            break;
        case hash("time"):
            offMode = SfzOffMode::time;
            break;
        default:
            DBG("Unkown off mode:" << opcode.value);
        }
        break;
    case hash("off_time"):
        offMode = SfzOffMode::time;
        setValueFromOpcode(opcode, offTime, Default::egTimeRange);
        break;
    case hash("polyphony"):
        if (auto value = readOpcode(opcode.value, Default::polyphonyRange))
            polyphony = *value;
        break;
    case hash("note_polyphony"):
        if (auto value = readOpcode(opcode.value, Default::polyphonyRange))
            notePolyphony = *value;
        break;
    case hash("note_selfmask"):
        switch (hash(opcode.value)) {
        case hash("on"):
            selfMask = SfzSelfMask::mask;
            break;
        case hash("off"):
            selfMask = SfzSelfMask::dontMask;
            break;
        default:
            DBG("Unkown self mask value:" << opcode.value);
        }
        break;
    case hash("rt_dead"):
        if (opcode.value == "on") {
            rtDead = true;
        } else if (opcode.value == "off") {
            rtDead = false;
        } else {
            DBG("Unkown rt_dead value:" << opcode.value);
        }
        break;
    // Region logic: key mapping
    case hash("lokey"):
        triggerOnNote = true;
        setRangeStartFromOpcode(opcode, keyRange, Default::keyRange);
        break;
    case hash("hikey"):
        triggerOnNote = (opcode.value != "-1");
        setRangeEndFromOpcode(opcode, keyRange, Default::keyRange);
        break;
    case hash("key"):
        triggerOnNote = (opcode.value != "-1");
        setRangeStartFromOpcode(opcode, keyRange, Default::keyRange);
        setRangeEndFromOpcode(opcode, keyRange, Default::keyRange);
        setValueFromOpcode(opcode, pitchKeycenter, Default::keyRange);
        break;
    case hash("lovel"):
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            velocityRange.setStart(normalizeVelocity(*value));
        break;
    case hash("hivel"):
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            velocityRange.setEnd(normalizeVelocity(*value));
        break;

    // Region logic: MIDI conditions
    case hash("lobend"):
        if (auto value = readOpcode(opcode.value, Default::bendRange))
            bendRange.setStart(normalizeBend(*value));
        break;
    case hash("hibend"):
        if (auto value = readOpcode(opcode.value, Default::bendRange))
            bendRange.setEnd(normalizeBend(*value));
        break;
    case hash("locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            ccConditions[opcode.parameters.back()].setStart(normalizeCC(*value));
        break;
    case hash("hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            ccConditions[opcode.parameters.back()].setEnd(normalizeCC(*value));
        break;
    case hash("lohdcc&"): // also lorealcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::normalizedRange))
            ccConditions[opcode.parameters.back()].setStart(*value);
        break;
    case hash("hihdcc&"): // also hirealcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::normalizedRange))
            ccConditions[opcode.parameters.back()].setEnd(*value);
        break;
    case hash("sw_lokey"): // fallthrough
    case hash("sw_hikey"):
        break;
    case hash("sw_last"):
        if (!lastKeyswitchRange) {
            setValueFromOpcode(opcode, lastKeyswitch, Default::keyRange);
            keySwitched = false;
        }
        break;
    case hash("sw_lolast"):
        if (auto value = readOpcode(opcode.value, Default::keyRange)) {
            if (!lastKeyswitchRange)
                lastKeyswitchRange.emplace(*value, *value);
            else
                lastKeyswitchRange->setStart(*value);

            keySwitched = false;
            lastKeyswitch = absl::nullopt;
        }
        break;
    case hash("sw_hilast"):
        if (auto value = readOpcode(opcode.value, Default::keyRange)) {
            if (!lastKeyswitchRange)
                lastKeyswitchRange.emplace(*value, *value);
            else
                lastKeyswitchRange->setEnd(*value);

            keySwitched = false;
            lastKeyswitch = absl::nullopt;
        }
        break;
    case hash("sw_label"):
        keyswitchLabel = opcode.value;
        break;
    case hash("sw_down"):
        setValueFromOpcode(opcode, downKeyswitch, Default::keyRange);
        keySwitched = false;
        break;
    case hash("sw_up"):
        setValueFromOpcode(opcode, upKeyswitch, Default::keyRange);
        break;
    case hash("sw_previous"):
        setValueFromOpcode(opcode, previousKeyswitch, Default::keyRange);
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
            DBG("Unknown velocity mode: " << opcode.value);
        }
        break;

    case hash("sustain_cc"):
        setValueFromOpcode(opcode, sustainCC, Default::ccNumberRange);
        break;
    case hash("sustain_lo"):
        if (auto value = readOpcode(opcode.value, Default::float7Range)) {
            sustainThreshold = normalizeCC(*value);
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
        sequenceSwitched = false;
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
            DBG("Unknown trigger mode: " << opcode.value);
        }
        break;
    case hash("start_locc&"): // also on_locc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range)) {
            triggerOnCC = true;
            ccTriggers[opcode.parameters.back()].setStart(normalizeCC(*value));
        }
        break;
    case hash("start_hicc&"): // also on_hicc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range)) {
            triggerOnCC = true;
            ccTriggers[opcode.parameters.back()].setEnd(normalizeCC(*value));
        }
        break;
    case hash("start_lohdcc&"): // also on_lohdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::normalizedRange)) {
            triggerOnCC = true;
            ccTriggers[opcode.parameters.back()].setStart(*value);
        }
        break;
    case hash("start_hihdcc&"): // also on_hihdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::normalizedRange)) {
            triggerOnCC = true;
            ccTriggers[opcode.parameters.back()].setEnd(*value);
        }
        break;

    // Performance parameters: amplifier
    case hash("volume"): // also gain
        setValueFromOpcode(opcode, volume, Default::volumeRange);
        break;
    case_any_ccN("volume"): // also gain
        processGenericCc(opcode, Default::volumeCCRange, ModKey::createNXYZ(ModId::Volume, id));
        break;
    case hash("amplitude"):
        if (auto value = readOpcode(opcode.value, Default::amplitudeRange))
            amplitude = normalizePercents(*value);
        break;
    case_any_ccN("amplitude"):
        processGenericCc(opcode, Default::amplitudeRange, ModKey::createNXYZ(ModId::Amplitude, id));
        break;
    case hash("pan"):
        if (auto value = readOpcode(opcode.value, Default::panRange))
            pan = normalizePercents(*value);
        break;
    case_any_ccN("pan"):
        processGenericCc(opcode, Default::panCCRange, ModKey::createNXYZ(ModId::Pan, id));
        break;
    case hash("position"):
        if (auto value = readOpcode(opcode.value, Default::positionRange))
            position = normalizePercents(*value);
        break;
    case_any_ccN("position"):
        processGenericCc(opcode, Default::positionCCRange, ModKey::createNXYZ(ModId::Position, id));
        break;
    case hash("width"):
        if (auto value = readOpcode(opcode.value, Default::widthRange))
            width = normalizePercents(*value);
        break;
    case_any_ccN("width"):
        processGenericCc(opcode, Default::widthCCRange, ModKey::createNXYZ(ModId::Width, id));
        break;
    case hash("amp_keycenter"):
        setValueFromOpcode(opcode, ampKeycenter, Default::keyRange);
        break;
    case hash("amp_keytrack"):
        setValueFromOpcode(opcode, ampKeytrack, Default::ampKeytrackRange);
        break;
    case hash("amp_veltrack"):
        if (auto value = readOpcode(opcode.value, Default::ampVeltrackRange))
            ampVeltrack = normalizePercents(*value);
        break;
    case hash("amp_random"):
        setValueFromOpcode(opcode, ampRandom, Default::ampRandomRange);
        break;
    case hash("amp_velcurve_&"):
        {
            auto value = readOpcode(opcode.value, Default::ampVelcurveRange);
            if (opcode.parameters.back() > 127)
                return false;

            const auto inputVelocity = static_cast<uint8_t>(opcode.parameters.back());
            if (value)
                velocityPoints.emplace_back(inputVelocity, *value);
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
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeVelInRange.setStart(normalizeVelocity(*value));
        break;
    case hash("xfin_hivel"):
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeVelInRange.setEnd(normalizeVelocity(*value));
        break;
    case hash("xfout_lovel"):
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeVelOutRange.setStart(normalizeVelocity(*value));
        break;
    case hash("xfout_hivel"):
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeVelOutRange.setEnd(normalizeVelocity(*value));
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
            DBG("Unknown crossfade power curve: " << opcode.value);
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
            DBG("Unknown crossfade power curve: " << opcode.value);
        }
        break;
    case hash("xfin_locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeCCInRange[opcode.parameters.back()].setStart(normalizeCC(*value));
        break;
    case hash("xfin_hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeCCInRange[opcode.parameters.back()].setEnd(normalizeCC(*value));
        break;
    case hash("xfout_locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeCCOutRange[opcode.parameters.back()].setStart(normalizeCC(*value));
        break;
    case hash("xfout_hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        if (auto value = readOpcode(opcode.value, Default::midi7Range))
            crossfadeCCOutRange[opcode.parameters.back()].setEnd(normalizeCC(*value));
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
            DBG("Unknown crossfade power curve: " << opcode.value);
        }
        break;
    case hash("rt_decay"):
        setValueFromOpcode(opcode, rtDecay, Default::rtDecayRange);
        break;
    case hash("global_amplitude"):
        if (auto value = readOpcode(opcode.value, Default::amplitudeRange))
            globalAmplitude = normalizePercents(*value);
        break;
    case hash("master_amplitude"):
        if (auto value = readOpcode(opcode.value, Default::amplitudeRange))
            masterAmplitude = normalizePercents(*value);
        break;
    case hash("group_amplitude"):
        if (auto value = readOpcode(opcode.value, Default::amplitudeRange))
            groupAmplitude = normalizePercents(*value);
        break;
    case hash("global_volume"):
        setValueFromOpcode(opcode, globalVolume, Default::volumeRange);
        break;
    case hash("master_volume"):
        setValueFromOpcode(opcode, masterVolume, Default::volumeRange);
        break;
    case hash("group_volume"):
        setValueFromOpcode(opcode, groupVolume, Default::volumeRange);
        break;

    // Performance parameters: filters
    case hash("cutoff&"): // also cutoff
        {
            const auto filterIndex = opcode.parameters.empty() ? 0 : (opcode.parameters.back() - 1);
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            setValueFromOpcode(opcode, filters[filterIndex].cutoff, Default::filterCutoffRange);
        }
        break;
    case hash("resonance&"): // also resonance
        {
            const auto filterIndex = opcode.parameters.empty() ? 0 : (opcode.parameters.back() - 1);
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            setValueFromOpcode(opcode, filters[filterIndex].resonance, Default::filterResonanceRange);
        }
        break;
    case_any_ccN("cutoff&"): // also cutoff_oncc&, cutoff_cc&, cutoff&_cc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterCutoffModRange, ModKey::createNXYZ(ModId::FilCutoff, id, filterIndex));
        }
        break;
    case_any_ccN("resonance&"): // also resonance_oncc&, resonance_cc&, resonance&_cc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterResonanceModRange, ModKey::createNXYZ(ModId::FilResonance, id, filterIndex));
        }
        break;
    case hash("fil&_keytrack"): // also fil_keytrack
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            setValueFromOpcode(opcode, filters[filterIndex].keytrack, Default::filterKeytrackRange);
        }
        break;
    case hash("fil&_keycenter"): // also fil_keycenter
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            setValueFromOpcode(opcode, filters[filterIndex].keycenter, Default::keyRange);
        }
        break;
    case hash("fil&_veltrack"): // also fil_veltrack
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            setValueFromOpcode(opcode, filters[filterIndex].veltrack, Default::filterVeltrackRange);
        }
        break;
    case hash("fil&_random"): // also fil_random, cutoff_random, cutoff&_random
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            setValueFromOpcode(opcode, filters[filterIndex].random, Default::filterRandomRange);
        }
        break;
    case hash("fil&_gain"): // also fil_gain
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            setValueFromOpcode(opcode, filters[filterIndex].gain, Default::filterGainRange);
        }
        break;
    case_any_ccN("fil&_gain"): // also fil_gain_oncc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterGainModRange, ModKey::createNXYZ(ModId::FilGain, id, filterIndex));
        }
        break;
    case hash("fil&_type"): // also fil_type, filtype
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            absl::optional<FilterType> ftype = Filter::typeFromName(opcode.value);

            if (ftype)
                filters[filterIndex].type = *ftype;
            else {
                filters[filterIndex].type = FilterType::kFilterNone;
                DBG("Unknown filter type: " << opcode.value);
            }
        }
        break;

    // Performance parameters: EQ
    case hash("eq&_bw"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            setValueFromOpcode(opcode, equalizers[eqIndex].bandwidth, Default::eqBandwidthRange);
        }
        break;
    case_any_ccN("eq&_bw"): // also eq&_bwcc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqBandwidthModRange, ModKey::createNXYZ(ModId::EqBandwidth, id, eqIndex));
        }
        break;
    case hash("eq&_freq"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            setValueFromOpcode(opcode, equalizers[eqIndex].frequency, Default::eqFrequencyRange);
        }
        break;
    case_any_ccN("eq&_freq"): // also eq&_freqcc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqFrequencyModRange, ModKey::createNXYZ(ModId::EqFrequency, id, eqIndex));
        }
        break;
    case hash("eq&_vel&freq"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (opcode.parameters[1] != 2)
                return false; // was eqN_vel3freq or something else than eqN_vel2freq
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            setValueFromOpcode(opcode, equalizers[eqIndex].vel2frequency, Default::eqFrequencyModRange);
        }
        break;
    case hash("eq&_gain"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            setValueFromOpcode(opcode, equalizers[eqIndex].gain, Default::eqGainRange);
        }
        break;
    case_any_ccN("eq&_gain"): // also eq&_gaincc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqGainModRange, ModKey::createNXYZ(ModId::EqGain, id, eqIndex));
        }
        break;
    case hash("eq&_vel&gain"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (opcode.parameters[1] != 2)
                return false;  // was eqN_vel3gain or something else than eqN_vel2gain
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            setValueFromOpcode(opcode, equalizers[eqIndex].vel2gain, Default::eqGainModRange);
        }
        break;
    case hash("eq&_type"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            absl::optional<EqType> ftype = FilterEq::typeFromName(opcode.value);

            if (ftype)
                equalizers[eqIndex].type = *ftype;
            else {
                equalizers[eqIndex].type = EqType::kEqNone;
                DBG("Unknown EQ type: " << opcode.value);
            }
        }
        break;

    // Performance parameters: pitch
    case hash("pitch_keycenter"):
        if (opcode.value == "sample")
            pitchKeycenterFromSample = true;
        else {
            pitchKeycenterFromSample = false;
            setValueFromOpcode(opcode, pitchKeycenter, Default::keyRange);
        }
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
    case hash("pitch"): // also tune
        setValueFromOpcode(opcode, tune, Default::tuneRange);
        break;
    case_any_ccN("pitch"): // also tune
        processGenericCc(opcode, Default::tuneCCRange, ModKey::createNXYZ(ModId::Pitch, id));
        break;
    case hash("bend_up"): // also bendup
        setValueFromOpcode(opcode, bendUp, Default::bendBoundRange);
        break;
    case hash("bend_down"): // also benddown
        setValueFromOpcode(opcode, bendDown, Default::bendBoundRange);
        break;
    case hash("bend_step"):
        setValueFromOpcode(opcode, bendStep, Default::bendStepRange);
        break;
    case hash("bend_smooth"):
        setValueFromOpcode(opcode, bendSmooth, Default::smoothCCRange);
        break;

    // Modulation: LFO
    case hash("lfo&_freq"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            setValueFromOpcode(opcode, lfos[lfoNumber - 1].freq, Default::lfoFreqRange);
        }
        break;
    case hash("lfo&_phase"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoPhaseRange))
                lfos[lfoNumber - 1].phase0 = wrapPhase(*value);
        }
        break;
    case hash("lfo&_delay"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            setValueFromOpcode(opcode, lfos[lfoNumber - 1].delay, Default::lfoDelayRange);
        }
        break;
    case hash("lfo&_fade"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            setValueFromOpcode(opcode, lfos[lfoNumber - 1].fade, Default::lfoFadeRange);
        }
        break;
    case hash("lfo&_count"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            setValueFromOpcode(opcode, lfos[lfoNumber - 1].count, Default::lfoCountRange);
        }
        break;
    case hash("lfo&_steps"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoStepsRange)) {
                if (!lfos[lfoNumber - 1].seq)
                    lfos[lfoNumber - 1].seq = LFODescription::StepSequence();
                lfos[lfoNumber - 1].seq->steps.resize(*value);
            }
        }
        break;
    case hash("lfo&_step&"):
        {
            const auto lfoNumber = opcode.parameters.front();
            const auto stepNumber = opcode.parameters[1];
            if (lfoNumber == 0 || stepNumber == 0 || stepNumber > config::maxLFOSteps)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoStepXRange)) {
                if (!lfos[lfoNumber - 1].seq)
                    lfos[lfoNumber - 1].seq = LFODescription::StepSequence();
                if (!extendIfNecessary(lfos[lfoNumber - 1].seq->steps, stepNumber, Default::numLFOSteps))
                    return false;
                lfos[lfoNumber - 1].seq->steps[stepNumber - 1] = *value * 0.01f;
            }
        }
        break;
    case hash("lfo&_wave&"): // also lfo&_wave
        {
            const auto lfoNumber = opcode.parameters.front();
            const auto subNumber = opcode.parameters[1];
            if (lfoNumber == 0 || subNumber == 0 || subNumber > config::maxLFOSubs)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoWaveRange)) {
                if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                    return false;
                lfos[lfoNumber - 1].sub[subNumber - 1].wave = static_cast<LFOWave>(*value);
            }
        }
        break;
    case hash("lfo&_offset&"): // also lfo&_offset
        {
            const auto lfoNumber = opcode.parameters.front();
            const auto subNumber = opcode.parameters[1];
            if (lfoNumber == 0 || subNumber == 0 || subNumber > config::maxLFOSubs)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoOffsetRange)) {
                if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                    return false;
                lfos[lfoNumber - 1].sub[subNumber - 1].offset = *value;
            }
        }
        break;
    case hash("lfo&_ratio&"): // also lfo&_ratio
        {
            const auto lfoNumber = opcode.parameters.front();
            const auto subNumber = opcode.parameters[1];
            if (lfoNumber == 0 || subNumber == 0 || subNumber > config::maxLFOSubs)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoRatioRange)) {
                if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                    return false;
                lfos[lfoNumber - 1].sub[subNumber - 1].ratio = *value;
            }
        }
        break;
    case hash("lfo&_scale&"): // also lfo&_scale
        {
            const auto lfoNumber = opcode.parameters.front();
            const auto subNumber = opcode.parameters[1];
            if (lfoNumber == 0 || subNumber == 0 || subNumber > config::maxLFOSubs)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (auto value = readOpcode(opcode.value, Default::lfoScaleRange)) {
                if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                    return false;
                lfos[lfoNumber - 1].sub[subNumber - 1].scale = *value;
            }
        }
        break;

    // Modulation: LFO (targets)
    case hash("lfo&_amplitude"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::amplitudeRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_pan"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::panCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_width"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::widthCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Width, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_position"): // sfizz extension
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::positionCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Position, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_pitch"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::tuneCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_volume"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::volumeCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("lfo&_cutoff&"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilCutoff, Default::filterCutoffModRange);
        break;
    case hash("lfo&_resonance&"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilResonance, Default::filterResonanceModRange);
        break;
    case hash("lfo&_fil&gain"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilGain, Default::filterGainModRange);
        break;
    case hash("lfo&_eq&gain"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqGain, Default::eqGainModRange);
        break;
    case hash("lfo&_eq&freq"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqFrequency, Default::eqFrequencyModRange);
        break;
    case hash("lfo&_eq&bw"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqBandwidth, Default::eqBandwidthModRange);
        break;

    // Modulation: Flex EG (targets)
    case hash("eg&_amplitude"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::amplitudeRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_pan"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::panCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_width"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::widthCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Width, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_position"): // sfizz extension
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::positionCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Position, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_pitch"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::tuneCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_volume"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (auto value = readOpcode(opcode.value, Default::volumeCCRange)) {
                const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
                const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
                getOrCreateConnection(source, target).sourceDepth = *value;
            }
        }
        break;
    case hash("eg&_cutoff&"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilCutoff, Default::filterCutoffModRange);
        break;
    case hash("eg&_resonance&"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilResonance, Default::filterResonanceModRange);
        break;
    case hash("eg&_fil&gain"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilGain, Default::filterGainModRange);
        break;
    case hash("eg&_eq&gain"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqGain, Default::eqGainModRange);
        break;
    case hash("eg&_eq&freq"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqFrequency, Default::eqFrequencyModRange);
        break;
    case hash("eg&_eq&bw"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqBandwidth, Default::eqBandwidthModRange);
        break;

    case hash("eg&_ampeg"):
    {
        const auto egNumber = opcode.parameters.front();
        if (egNumber == 0)
            return false;
        if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
            return false;
        if (auto ampeg = readBooleanFromOpcode(opcode)) {
            FlexEGDescription& desc = flexEGs[egNumber - 1];
            if (desc.ampeg != *ampeg) {
                desc.ampeg = *ampeg;
                flexAmpEG = absl::nullopt;
                for (size_t i = 0, n = flexEGs.size(); i < n && !flexAmpEG; ++i) {
                    if (flexEGs[i].ampeg)
                        flexAmpEG = static_cast<uint8_t>(i);
                }
            }
        }
        break;
    }

    // Amplitude Envelope
    case hash("ampeg_attack"):
    case hash("ampeg_decay"):
    case hash("ampeg_delay"):
    case hash("ampeg_hold"):
    case hash("ampeg_release"):
    case hash("ampeg_start"):
    case hash("ampeg_sustain"):
    case hash("ampeg_vel&attack"):
    case hash("ampeg_vel&decay"):
    case hash("ampeg_vel&delay"):
    case hash("ampeg_vel&hold"):
    case hash("ampeg_vel&release"):
    case hash("ampeg_vel&sustain"):
    case hash("ampeg_attack_oncc&"): // also ampeg_attackcc&
    case hash("ampeg_decay_oncc&"): // also ampeg_decaycc&
    case hash("ampeg_delay_oncc&"): // also ampeg_delaycc&
    case hash("ampeg_hold_oncc&"): // also ampeg_holdcc&
    case hash("ampeg_release_oncc&"): // also ampeg_releasecc&
    case hash("ampeg_start_oncc&"): // also ampeg_startcc&
    case hash("ampeg_sustain_oncc&"): // also ampeg_sustaincc&
        parseEGOpcode(opcode, amplitudeEG);
        break;

    case hash("pitcheg_attack"):
    case hash("pitcheg_decay"):
    case hash("pitcheg_delay"):
    case hash("pitcheg_hold"):
    case hash("pitcheg_release"):
    case hash("pitcheg_start"):
    case hash("pitcheg_sustain"):
    case hash("pitcheg_vel&attack"):
    case hash("pitcheg_vel&decay"):
    case hash("pitcheg_vel&delay"):
    case hash("pitcheg_vel&hold"):
    case hash("pitcheg_vel&release"):
    case hash("pitcheg_vel&sustain"):
    case hash("pitcheg_attack_oncc&"): // also pitcheg_attackcc&
    case hash("pitcheg_decay_oncc&"): // also pitcheg_decaycc&
    case hash("pitcheg_delay_oncc&"): // also pitcheg_delaycc&
    case hash("pitcheg_hold_oncc&"): // also pitcheg_holdcc&
    case hash("pitcheg_release_oncc&"): // also pitcheg_releasecc&
    case hash("pitcheg_start_oncc&"): // also pitcheg_startcc&
    case hash("pitcheg_sustain_oncc&"): // also pitcheg_sustaincc&
        if (parseEGOpcode(opcode, pitchEG))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::PitchEG, id),
                ModKey::createNXYZ(ModId::Pitch, id));
        break;

    case hash("fileg_attack"):
    case hash("fileg_decay"):
    case hash("fileg_delay"):
    case hash("fileg_hold"):
    case hash("fileg_release"):
    case hash("fileg_start"):
    case hash("fileg_sustain"):
    case hash("fileg_vel&attack"):
    case hash("fileg_vel&decay"):
    case hash("fileg_vel&delay"):
    case hash("fileg_vel&hold"):
    case hash("fileg_vel&release"):
    case hash("fileg_vel&sustain"):
    case hash("fileg_attack_oncc&"): // also fileg_attackcc&
    case hash("fileg_decay_oncc&"): // also fileg_decaycc&
    case hash("fileg_delay_oncc&"): // also fileg_delaycc&
    case hash("fileg_hold_oncc&"): // also fileg_holdcc&
    case hash("fileg_release_oncc&"): // also fileg_releasecc&
    case hash("fileg_start_oncc&"): // also fileg_startcc&
    case hash("fileg_sustain_oncc&"): // also fileg_sustaincc&
        if (parseEGOpcode(opcode, filterEG))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::FilEG, id),
                ModKey::createNXYZ(ModId::FilCutoff, id));
        break;

    case hash("pitcheg_depth"):
        if (auto value = readOpcode(opcode.value, Default::pitchEgDepthRange))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::PitchEG, id),
                ModKey::createNXYZ(ModId::Pitch, id)).sourceDepth = *value;
        break;
    case hash("fileg_depth"):
        if (auto value = readOpcode(opcode.value, Default::filterEgDepthRange))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::FilEG, id),
                ModKey::createNXYZ(ModId::FilCutoff, id)).sourceDepth = *value;
        break;

    case hash("pitcheg_vel&depth"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        if (auto value = readOpcode(opcode.value, Default::pitchEgDepthRange))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::PitchEG, id),
                ModKey::createNXYZ(ModId::Pitch, id)).velToDepth = *value;
        break;
    case hash("fileg_vel&depth"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        if (auto value = readOpcode(opcode.value, Default::filterEgDepthRange))
            getOrCreateConnection(
                ModKey::createNXYZ(ModId::FilEG, id),
                ModKey::createNXYZ(ModId::FilCutoff, id)).velToDepth = *value;
        break;

    // Flex envelopes
    case hash("eg&_dynamic"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            setValueFromOpcode(opcode, eg.dynamic, Default::flexEGDynamicRange);
        }
        break;
    case hash("eg&_sustain"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            setValueFromOpcode(opcode, eg.sustain, Default::flexEGSustainRange);
        }
        break;
    case hash("eg&_time&"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            const auto pointNumber = opcode.parameters[1];
            if (!extendIfNecessary(eg.points, pointNumber + 1, Default::numFlexEGPoints))
                return false;
            setValueFromOpcode(opcode, eg.points[pointNumber].time, Default::flexEGPointTimeRange);
        }
        break;
    case hash("eg&_level&"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            const auto pointNumber = opcode.parameters[1];
            if (!extendIfNecessary(eg.points, pointNumber + 1, Default::numFlexEGPoints))
                return false;
            setValueFromOpcode(opcode, eg.points[pointNumber].level, Default::flexEGPointLevelRange);
        }
        break;
    case hash("eg&_shape&"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            const auto pointNumber = opcode.parameters[1];
            if (!extendIfNecessary(eg.points, pointNumber + 1, Default::numFlexEGPoints))
                return false;
            if (auto value = readOpcode(opcode.value, Default::flexEGPointShapeRange))
                eg.points[pointNumber].setShape(*value);
        }
        break;

    case hash("effect&"):
    {
        const auto effectNumber = opcode.parameters.back();
        if (!effectNumber || effectNumber < 1 || effectNumber > config::maxEffectBuses)
            break;
        auto value = readOpcode<float>(opcode.value, { 0, 100 });
        if (!value)
            break;
        if (static_cast<size_t>(effectNumber + 1) > gainToEffect.size())
            gainToEffect.resize(effectNumber + 1);
        gainToEffect[effectNumber] = *value / 100;
        break;
    }
    case hash("sw_default"):
        setValueFromOpcode(opcode, defaultSwitch, Default::keyRange);
        break;

    // Ignored opcodes
    case hash("hichan"):
    case hash("lochan"):
    case hash("ampeg_depth"):
    case hash("ampeg_vel&depth"):
        break;
    default:
        return false;

    #undef case_any_ccN
    #undef LFO_EG_filter_EQ_target
    }

    return true;
}

bool sfz::Region::parseEGOpcode(const Opcode& opcode, EGDescription& eg)
{
    #define case_any_eg(param)                      \
        case hash("ampeg_" param):                  \
        case hash("pitcheg_" param):                \
        case hash("fileg_" param)                   \

    switch (opcode.lettersOnlyHash) {
    case_any_eg("attack"):
        setValueFromOpcode(opcode, eg.attack, Default::egTimeRange);
        break;
    case_any_eg("decay"):
        setValueFromOpcode(opcode, eg.decay, Default::egTimeRange);
        break;
    case_any_eg("delay"):
        setValueFromOpcode(opcode, eg.delay, Default::egTimeRange);
        break;
    case_any_eg("hold"):
        setValueFromOpcode(opcode, eg.hold, Default::egTimeRange);
        break;
    case_any_eg("release"):
        setValueFromOpcode(opcode, eg.release, Default::egTimeRange);
        break;
    case_any_eg("start"):
        setValueFromOpcode(opcode, eg.start, Default::egPercentRange);
        break;
    case_any_eg("sustain"):
        setValueFromOpcode(opcode, eg.sustain, Default::egPercentRange);
        break;
    case_any_eg("vel&attack"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2attack, Default::egOnCCTimeRange);
        break;
    case_any_eg("vel&decay"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2decay, Default::egOnCCTimeRange);
        break;
    case_any_eg("vel&delay"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2delay, Default::egOnCCTimeRange);
        break;
    case_any_eg("vel&hold"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2hold, Default::egOnCCTimeRange);
        break;
    case_any_eg("vel&release"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2release, Default::egOnCCTimeRange);
        break;
    case_any_eg("vel&sustain"):
        if (opcode.parameters.front() != 2)
            return false; // Was not vel2...
        setValueFromOpcode(opcode, eg.vel2sustain, Default::egOnCCPercentRange);
        break;
    case_any_eg("attack_oncc&"): // also attackcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCTimeRange))
            eg.ccAttack[opcode.parameters.back()] = *value;

        break;
    case_any_eg("decay_oncc&"): // also decaycc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCTimeRange))
            eg.ccDecay[opcode.parameters.back()] = *value;

        break;
    case_any_eg("delay_oncc&"): // also delaycc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCTimeRange))
            eg.ccDelay[opcode.parameters.back()] = *value;

        break;
    case_any_eg("hold_oncc&"): // also holdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCTimeRange))
            eg.ccHold[opcode.parameters.back()] = *value;

        break;
    case_any_eg("release_oncc&"): // also releasecc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCTimeRange))
            eg.ccRelease[opcode.parameters.back()] = *value;

        break;
    case_any_eg("start_oncc&"): // also startcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCPercentRange))
            eg.ccStart[opcode.parameters.back()] = *value;

        break;
    case_any_eg("sustain_oncc&"): // also sustaincc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        if (auto value = readOpcode(opcode.value, Default::egOnCCPercentRange))
            eg.ccSustain[opcode.parameters.back()] = *value;

        break;
    default:
        return false;
    }

    return true;

    #undef case_any_eg
}

bool sfz::Region::parseEGOpcode(const Opcode& opcode, absl::optional<EGDescription>& eg)
{
    bool create = eg == absl::nullopt;
    if (create)
        eg = EGDescription();

    bool parsed = parseEGOpcode(opcode, *eg);
    if (!parsed && create)
        eg = absl::nullopt;

    return parsed;
}

bool sfz::Region::processGenericCc(const Opcode& opcode, Range<float> range, const ModKey& target)
{
    if (!opcode.isAnyCcN())
        return false;

    const auto ccNumber = opcode.parameters.back();
    if (ccNumber >= config::numCCs)
        return false;

    if (target) {
        // search an existing connection of same CC number and target
        // if it exists, modify, otherwise create
        auto it = std::find_if(connections.begin(), connections.end(),
            [ccNumber, &target](const Connection& x) -> bool
            {
                return x.source.id() == ModId::Controller &&
                    x.source.parameters().cc == ccNumber &&
                    x.target == target;
            });

        Connection *conn;
        if (it != connections.end())
            conn = &*it;
        else {
            connections.emplace_back();
            conn = &connections.back();
            conn->source = ModKey::createCC(ccNumber, 0, 0, 0);
            conn->target = target;
        }

        //
        ModKey::Parameters p = conn->source.parameters();
        switch (opcode.category) {
        case kOpcodeOnCcN:
            setValueFromOpcode(opcode, conn->sourceDepth, range);
            break;
        case kOpcodeCurveCcN:
            setValueFromOpcode(opcode, p.curve, Default::curveCCRange);
            break;
        case kOpcodeStepCcN:
            {
                const Range<float> stepCCRange { 0.0f, std::max(std::abs(range.getStart()), std::abs(range.getEnd())) };
                setValueFromOpcode(opcode, p.step, stepCCRange);
            }
            break;
        case kOpcodeSmoothCcN:
            setValueFromOpcode(opcode, p.smooth, Default::smoothCCRange);
            break;
        default:
            assert(false);
            break;
        }
        conn->source = ModKey(ModId::Controller, {}, p);
    }

    return true;
}

bool sfz::Region::isSwitchedOn() const noexcept
{
    return keySwitched && previousKeySwitched && sequenceSwitched && pitchSwitched && bpmSwitched && aftertouchSwitched && ccSwitched.all();
}

bool sfz::Region::registerNoteOn(int noteNumber, float velocity, float randValue) noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    const bool keyOk = keyRange.containsWithEnd(noteNumber);
    if (keyOk) {
        // Sequence activation
        sequenceSwitched =
            ((sequenceCounter++ % sequenceLength) == sequencePosition - 1);
    }

    if (!isSwitchedOn())
        return false;

    if (!triggerOnNote)
        return false;

    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue) || (randValue == 1.0f && randRange.getEnd() == 1.0f);
    const bool firstLegatoNote = (trigger == SfzTrigger::first && midiState.getActiveNotes() == 1);
    const bool attackTrigger = (trigger == SfzTrigger::attack);
    const bool notFirstLegatoNote = (trigger == SfzTrigger::legato && midiState.getActiveNotes() > 1);

    return keyOk && velOk && randOk && (attackTrigger || firstLegatoNote || notFirstLegatoNote);
}

bool sfz::Region::registerNoteOff(int noteNumber, float velocity, float randValue) noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    if (!isSwitchedOn())
        return false;

    if (!triggerOnNote)
        return false;

    // Prerequisites

    const bool keyOk = keyRange.containsWithEnd(noteNumber);
    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue);

    if (!(velOk && keyOk && randOk))
        return false;

    // Release logic

    if (trigger == SfzTrigger::release_key)
        return true;

    if (trigger == SfzTrigger::release) {
        if (midiState.getCCValue(sustainCC) < sustainThreshold)
            return true;

        // If we reach this part, we're storing the notes to delay their release on CC up
        // This is handled by the Synth object

        delayedReleases.emplace_back(noteNumber, midiState.getNoteVelocity(noteNumber));
    }

    return false;
}

bool sfz::Region::registerCC(int ccNumber, float ccValue) noexcept
{
    ASSERT(ccValue >= 0.0f && ccValue <= 1.0f);
    if (ccConditions.getWithDefault(ccNumber).containsWithEnd(ccValue))
        ccSwitched.set(ccNumber, true);
    else
        ccSwitched.set(ccNumber, false);

    if (!isSwitchedOn())
        return false;

    if (!triggerOnCC)
        return false;

    if (ccTriggers.contains(ccNumber) && ccTriggers[ccNumber].containsWithEnd(ccValue))
        return true;

    return false;
}

void sfz::Region::registerPitchWheel(float pitch) noexcept
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

float sfz::Region::getBasePitchVariation(float noteNumber, float velocity) const noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    fast_real_distribution<float> pitchDistribution { -pitchRandom, pitchRandom };
    auto pitchVariationInCents = pitchKeytrack * (noteNumber - pitchKeycenter); // note difference with pitch center
    pitchVariationInCents += tune; // sample tuning
    pitchVariationInCents += config::centPerSemitone * transpose; // sample transpose
    pitchVariationInCents += velocity * pitchVeltrack; // track velocity
    pitchVariationInCents += pitchDistribution(Random::randomGenerator); // random pitch changes
    return centsFactor(pitchVariationInCents);
}

float sfz::Region::getBaseVolumedB(int noteNumber) const noexcept
{
    fast_real_distribution<float> volumeDistribution { -ampRandom, ampRandom };
    auto baseVolumedB = volume + volumeDistribution(Random::randomGenerator);
    baseVolumedB += globalVolume;
    baseVolumedB += masterVolume;
    baseVolumedB += groupVolume;
    if (trigger == SfzTrigger::release || trigger == SfzTrigger::release_key)
        baseVolumedB -= rtDecay * midiState.getNoteDuration(noteNumber);
    return baseVolumedB;
}

float sfz::Region::getBaseGain() const noexcept
{
    float baseGain = amplitude;

    baseGain *= globalAmplitude;
    baseGain *= masterAmplitude;
    baseGain *= groupAmplitude;

    return baseGain;
}

float sfz::Region::getPhase() const noexcept
{
    float phase;
    if (oscillatorPhase >= 0)
        phase = oscillatorPhase;
    else {
        fast_real_distribution<float> phaseDist { 0.0001f, 0.9999f };
        phase = phaseDist(Random::randomGenerator);
    }
    return phase;
}

uint64_t sfz::Region::getOffset(Oversampling factor) const noexcept
{
    std::uniform_int_distribution<int64_t> offsetDistribution { 0, offsetRandom };
    uint64_t finalOffset = offset + offsetDistribution(Random::randomGenerator);
    for (const auto& mod: offsetCC)
        finalOffset += static_cast<uint64_t>(mod.data * midiState.getCCValue(mod.cc));
    return Default::offsetRange.clamp(finalOffset) * static_cast<uint64_t>(factor);
}

float sfz::Region::getDelay() const noexcept
{
    fast_real_distribution<float> delayDistribution { 0, delayRandom };
    return delay + delayDistribution(Random::randomGenerator);
}

uint32_t sfz::Region::trueSampleEnd(Oversampling factor) const noexcept
{
    if (sampleEnd <= 0)
        return 0;

    return min(static_cast<uint32_t>(sampleEnd), loopRange.getEnd()) * static_cast<uint32_t>(factor);
}

uint32_t sfz::Region::loopStart(Oversampling factor) const noexcept
{
    return loopRange.getStart() * static_cast<uint32_t>(factor);
}

uint32_t sfz::Region::loopEnd(Oversampling factor) const noexcept
{
    return loopRange.getEnd() * static_cast<uint32_t>(factor);
}

float sfz::Region::getNoteGain(int noteNumber, float velocity) const noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

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

float sfz::Region::getCrossfadeGain() const noexcept
{
    float gain { 1.0f };

    // Crossfades due to CC states
    for (const auto& ccData : crossfadeCCInRange) {
        const auto ccValue = midiState.getCCValue(ccData.cc);
        const auto crossfadeRange = ccData.data;
        gain *= crossfadeIn(crossfadeRange, ccValue, crossfadeCCCurve);
    }

    for (const auto& ccData : crossfadeCCOutRange) {
        const auto ccValue = midiState.getCCValue(ccData.cc);
        const auto crossfadeRange = ccData.data;
        gain *= crossfadeOut(crossfadeRange, ccValue, crossfadeCCCurve);
    }

    return gain;
}

float sfz::Region::velocityCurve(float velocity) const noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    float gain;
    if (velCurve)
        gain = velCurve->evalNormalized(velocity);
    else
        gain = velocity * velocity;

    gain = std::fabs(ampVeltrack) * (1.0f - gain);
    gain = (ampVeltrack < 0) ? gain : (1.0f - gain);

    return gain;
}

void sfz::Region::offsetAllKeys(int offset) noexcept
{
    // Offset key range
    if (keyRange != Default::keyRange) {
        const auto start = keyRange.getStart();
        const auto end = keyRange.getEnd();
        keyRange.setStart(offsetAndClampKey(start, offset, Default::keyRange));
        keyRange.setEnd(offsetAndClampKey(end, offset, Default::keyRange));
    }
    pitchKeycenter = offsetAndClampKey(pitchKeycenter, offset, Default::keyRange);

    // Offset key switches
    if (upKeyswitch)
        upKeyswitch = offsetAndClampKey(*upKeyswitch, offset, Default::keyRange);
    if (lastKeyswitch)
        lastKeyswitch = offsetAndClampKey(*lastKeyswitch, offset, Default::keyRange);
    if (downKeyswitch)
        downKeyswitch = offsetAndClampKey(*downKeyswitch, offset, Default::keyRange);
    if (previousKeyswitch)
        previousKeyswitch = offsetAndClampKey(*previousKeyswitch, offset, Default::keyRange);

    // Offset crossfade ranges
    if (crossfadeKeyInRange != Default::crossfadeKeyInRange) {
        const auto start = crossfadeKeyInRange.getStart();
        const auto end = crossfadeKeyInRange.getEnd();
        crossfadeKeyInRange.setStart(offsetAndClampKey(start, offset, Default::keyRange));
        crossfadeKeyInRange.setEnd(offsetAndClampKey(end, offset, Default::keyRange));
    }

    if (crossfadeKeyOutRange != Default::crossfadeKeyOutRange) {
        const auto start = crossfadeKeyOutRange.getStart();
        const auto end = crossfadeKeyOutRange.getEnd();
        crossfadeKeyOutRange.setStart(offsetAndClampKey(start, offset, Default::keyRange));
        crossfadeKeyOutRange.setEnd(offsetAndClampKey(end, offset, Default::keyRange));
    }
}

float sfz::Region::getGainToEffectBus(unsigned number) const noexcept
{
    if (number >= gainToEffect.size())
        return 0.0;

    return gainToEffect[number];
}

float sfz::Region::getBendInCents(float bend) const noexcept
{
    return bend > 0.0f ? bend * static_cast<float>(bendUp) : -bend * static_cast<float>(bendDown);
}

sfz::Region::Connection* sfz::Region::getConnection(const ModKey& source, const ModKey& target)
{
    auto pred = [&source, &target](const Connection& c)
    {
        return c.source == source && c.target == target;
    };

    auto it = std::find_if(connections.begin(), connections.end(), pred);
    return (it == connections.end()) ? nullptr : &*it;
}

sfz::Region::Connection& sfz::Region::getOrCreateConnection(const ModKey& source, const ModKey& target)
{
    if (Connection* c = getConnection(source, target))
        return *c;

    sfz::Region::Connection c;
    c.source = source;
    c.target = target;

    connections.push_back(c);
    return connections.back();
}

bool sfz::Region::disabled() const noexcept
{
    return (sampleEnd == 0);
}

absl::optional<float> sfz::Region::ccModDepth(int cc, ModId id) const noexcept
{
    const ModKey target = ModKey::createNXYZ(id, getId());
    for (const sfz::Region::Connection& conn : connections) {
        if (conn.source.id() == sfz::ModId::Controller && conn.target == target) {
            auto p = conn.source.parameters();
            if (p.cc == cc)
                return conn.sourceDepth;
        }
    }

    return {};
}

absl::optional<sfz::ModKey::Parameters> sfz::Region::ccModParameters(int cc, ModId id) const noexcept
{
    const ModKey target = ModKey::createNXYZ(id, getId());
    for (const sfz::Region::Connection& conn : connections) {
        if (conn.source.id() == sfz::ModId::Controller && conn.target == target) {
            auto p = conn.source.parameters();
            if (p.cc == cc)
                return p;
        }
    }

    return {};
}
