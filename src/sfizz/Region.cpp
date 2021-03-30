// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Region.h"
#include "Opcode.h"
#include "MathHelpers.h"
#include "utility/SwapAndPop.h"
#include "utility/StringViewHelpers.h"
#include "utility/Macros.h"
#include "utility/Debug.h"
#include "ModifierHelpers.h"
#include "modulations/ModId.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/match.h"
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

sfz::Region::Region(int regionNumber, const MidiState& midiState, absl::string_view defaultPath)
: id{regionNumber}, midiState(midiState), defaultPath(defaultPath)
{
    ccSwitched.set();

    gainToEffect.reserve(5); // sufficient room for main and fx1-4
    gainToEffect.push_back(1.0); // contribute 100% into the main bus

    // Default amplitude release
    amplitudeEG.release = Default::egRelease;
}

// Helper for ccN processing
#define case_any_ccN(x)        \
    case hash(x "_oncc&"):     \
    case hash(x "_curvecc&"):  \
    case hash(x "_stepcc&"):   \
    case hash(x "_smoothcc&")

bool sfz::Region::parseOpcode(const Opcode& rawOpcode)
{
    const Opcode opcode = rawOpcode.cleanUp(kOpcodeScopeRegion);

    switch (opcode.lettersOnlyHash) {

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
        sampleQuality = opcode.read(Default::sampleQuality);
        break;
    case hash("direction"):
        *sampleId = sampleId->reversed(opcode.value == "reverse");
        break;
    case hash("delay"):
        delay = opcode.read(Default::delay);
        break;
    case hash("delay_oncc&"): // also delay_cc&
        if (opcode.parameters.back() > config::numCCs)
            return false;

        delayCC[opcode.parameters.back()] = opcode.read(Default::delayMod);
        break;
    case hash("delay_random"):
        delayRandom = opcode.read(Default::delayRandom);
        break;
    case hash("offset"):
        offset = opcode.read(Default::offset);
        break;
    case hash("offset_random"):
        offsetRandom = opcode.read(Default::offsetRandom);
        break;
    case hash("offset_oncc&"): // also offset_cc&
        if (opcode.parameters.back() > config::numCCs)
            return false;

        offsetCC[opcode.parameters.back()] = opcode.read(Default::offsetMod);
        break;
    case hash("end"):
        sampleEnd = opcode.read(Default::sampleEnd);
        break;
    case hash("count"):
        sampleCount = opcode.readOptional(Default::sampleCount);
        loopMode = LoopMode::one_shot;
        break;
    case hash("loop_mode"): // also loopmode
        loopMode = opcode.readOptional(Default::loopMode);
        break;
    case hash("loop_end"): // also loopend
        loopRange.setEnd(opcode.read(Default::loopEnd));
        break;
    case hash("loop_count"):
        loopCount = opcode.readOptional(Default::loopCount);
        break;
    case hash("loop_start"): // also loopstart
        loopRange.setStart(opcode.read(Default::loopStart));
        break;
    case hash("loop_crossfade"):
        loopCrossfade = opcode.read(Default::loopCrossfade);
        break;

    // Wavetable oscillator
    case hash("oscillator_phase"):
        {
            auto phase = opcode.read(Default::oscillatorPhase);
            oscillatorPhase = (phase >= 0) ? wrapPhase(phase) : -1.0f;
        }
        break;
    case hash("oscillator"):
        oscillatorEnabled = opcode.read(Default::oscillator);
        break;
    case hash("oscillator_mode"):
        oscillatorMode = opcode.read(Default::oscillatorMode);
        break;
    case hash("oscillator_multi"):
        oscillatorMulti = opcode.read(Default::oscillatorMulti);
        break;
    case hash("oscillator_detune"):
        oscillatorDetune = opcode.read(Default::oscillatorDetune);
        break;
    case_any_ccN("oscillator_detune"):
        processGenericCc(opcode, Default::oscillatorDetuneMod,
            ModKey::createNXYZ(ModId::OscillatorDetune, id));
        break;
    case hash("oscillator_mod_depth"):
        oscillatorModDepth = opcode.read(Default::oscillatorModDepth);
        break;
    case_any_ccN("oscillator_mod_depth"):
        processGenericCc(opcode, Default::oscillatorModDepthMod,
            ModKey::createNXYZ(ModId::OscillatorModDepth, id));
        break;
    case hash("oscillator_quality"):
        oscillatorQuality = opcode.readOptional(Default::oscillatorQuality);
        break;

    // Instrument settings: voice lifecycle
    case hash("group"): // also polyphony_group
        group = opcode.read(Default::group);
        break;
    case hash("off_by"): // also offby
        offBy = opcode.readOptional(Default::group);
        break;
    case hash("off_mode"): // also offmode
         offMode = opcode.read(Default::offMode);
        break;
    case hash("off_time"):
        offMode = OffMode::time;
        offTime = opcode.read(Default::offTime);
        break;
    case hash("polyphony"):
        polyphony = opcode.read(Default::polyphony);
        break;
    case hash("note_polyphony"):
        notePolyphony = opcode.read(Default::notePolyphony);
        break;
    case hash("note_selfmask"):
        selfMask = opcode.read(Default::selfMask);
        break;
    case hash("rt_dead"):
        rtDead = opcode.read(Default::rtDead);
        break;
    // Region logic: key mapping
    case hash("lokey"):
        triggerOnNote = true;
        keyRange.setStart(opcode.read(Default::loKey));
        break;
    case hash("hikey"):
        {
            absl::optional<uint8_t> optValue = opcode.readOptional(Default::hiKey);
            triggerOnNote = optValue != absl::nullopt;
            uint8_t value = optValue.value_or(Default::hiKey);
            keyRange.setEnd(value);
        }
        break;
    case hash("key"):
        {
            absl::optional<uint8_t> optValue = opcode.readOptional(Default::key);
            triggerOnNote = optValue != absl::nullopt;
            uint8_t value = optValue.value_or(Default::key);
            keyRange.setStart(value);
            keyRange.setEnd(value);
            pitchKeycenter = value;
        }
        break;
    case hash("lovel"):
        velocityRange.setStart(opcode.read(Default::loVel));
        break;
    case hash("hivel"):
        velocityRange.setEnd(opcode.read(Default::hiVel));
        break;

    // Region logic: MIDI conditions
    case hash("lobend"):
        bendRange.setStart(opcode.read(Default::loBend));
        break;
    case hash("hibend"):
        bendRange.setEnd(opcode.read(Default::hiBend));
        break;
    case hash("locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        ccConditions[opcode.parameters.back()].setStart(
            opcode.read(Default::loCC)
        );
        break;
    case hash("hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        ccConditions[opcode.parameters.back()].setEnd(
            opcode.read(Default::hiCC)
        );
        break;
    case hash("lohdcc&"): // also lorealcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        ccConditions[opcode.parameters.back()].setStart(
            opcode.read(Default::loNormalized)
        );
        break;
    case hash("hihdcc&"): // also hirealcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        ccConditions[opcode.parameters.back()].setEnd(
            opcode.read(Default::hiNormalized)
        );
        break;
    case hash("sw_lokey"): // fallthrough
    case hash("sw_hikey"):
        break;
    case hash("sw_last"):
        if (!lastKeyswitchRange) {
            lastKeyswitch = opcode.readOptional(Default::key);
            keySwitched = !lastKeyswitch.has_value();
        }
        break;
    case hash("sw_lolast"):
        {
            auto value = opcode.read(Default::key);
            if (!lastKeyswitchRange)
                lastKeyswitchRange.emplace(value, value);
            else
                lastKeyswitchRange->setStart(value);

            keySwitched = false;
            lastKeyswitch = absl::nullopt;
        }
        break;
    case hash("sw_hilast"):
        {
            auto value = opcode.read(Default::key);
            if (!lastKeyswitchRange)
                lastKeyswitchRange.emplace(value, value);
            else
                lastKeyswitchRange->setEnd(value);

            keySwitched = false;
            lastKeyswitch = absl::nullopt;
        }
        break;
    case hash("sw_label"):
        keyswitchLabel = opcode.value;
        break;
    case hash("sw_down"):
        downKeyswitch = opcode.readOptional(Default::key);
        keySwitched = !downKeyswitch.has_value();
        break;
    case hash("sw_up"):
        upKeyswitch = opcode.readOptional(Default::key);
        break;
    case hash("sw_previous"):
        previousKeyswitch = opcode.readOptional(Default::key);
        previousKeySwitched = !previousKeyswitch.has_value();
        break;
    case hash("sw_vel"):
        velocityOverride =
            opcode.read(Default::velocityOverride);
        break;

    case hash("sustain_cc"):
        sustainCC = opcode.read(Default::sustainCC);
        break;
    case hash("sostenuto_cc"):
        sostenutoCC = opcode.read(Default::sostenutoCC);
        break;
    case hash("sustain_lo"):
        sustainThreshold = opcode.read(Default::sustainThreshold);
        break;
    case hash("sostenuto_lo"):
        sostenutoThreshold = opcode.read(Default::sostenutoThreshold);
        break;
    case hash("sustain_sw"):
        checkSustain = opcode.read(Default::checkSustain);
        break;
    case hash("sostenuto_sw"):
        checkSostenuto = opcode.read(Default::checkSostenuto);
        break;
    // Region logic: internal conditions
    case hash("lochanaft"):
        aftertouchRange.setStart(opcode.read(Default::loChannelAftertouch));
        break;
    case hash("hichanaft"):
        aftertouchRange.setEnd(opcode.read(Default::hiChannelAftertouch));
        break;
    case hash("lobpm"):
        bpmRange.setStart(opcode.read(Default::loBPM));
        break;
    case hash("hibpm"):
        bpmRange.setEnd(opcode.read(Default::hiBPM));
        break;
    case hash("lorand"):
        randRange.setStart(opcode.read(Default::loNormalized));
        break;
    case hash("hirand"):
        randRange.setEnd(opcode.read(Default::hiNormalized));
        break;
    case hash("seq_length"):
        sequenceLength = opcode.read(Default::sequence);
        break;
    case hash("seq_position"):
        sequencePosition = opcode.read(Default::sequence);
        sequenceSwitched = false;
        break;
    // Region logic: triggers
    case hash("trigger"):
        trigger = opcode.read(Default::trigger);
        break;
    case hash("start_locc&"): // also on_locc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        triggerOnCC = true;
        ccTriggers[opcode.parameters.back()].setStart(
            opcode.read(Default::loCC)
        );
        break;
    case hash("start_hicc&"): // also on_hicc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        triggerOnCC = true;
        ccTriggers[opcode.parameters.back()].setEnd(
            opcode.read(Default::hiCC)
        );
        break;
    case hash("start_lohdcc&"): // also on_lohdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        triggerOnCC = true;
        ccTriggers[opcode.parameters.back()].setStart(
            opcode.read(Default::loNormalized)
        );
        break;
    case hash("start_hihdcc&"): // also on_hihdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        ccTriggers[opcode.parameters.back()].setEnd(
            opcode.read(Default::hiNormalized)
        );
        break;

    // Performance parameters: amplifier
    case hash("volume"): // also gain
        volume = opcode.read(Default::volume);
        break;
    case_any_ccN("volume"): // also gain
        processGenericCc(opcode, Default::volumeMod, ModKey::createNXYZ(ModId::Volume, id));
        break;
    case hash("amplitude"):
        amplitude = opcode.read(Default::amplitude);
        break;
    case_any_ccN("amplitude"):
        processGenericCc(opcode, Default::amplitudeMod, ModKey::createNXYZ(ModId::Amplitude, id));
        break;
    case hash("pan"):
        pan = opcode.read(Default::pan);
        break;
    case_any_ccN("pan"):
        processGenericCc(opcode, Default::panMod, ModKey::createNXYZ(ModId::Pan, id));
        break;
    case hash("position"):
        position = opcode.read(Default::position);
        break;
    case_any_ccN("position"):
        processGenericCc(opcode, Default::positionMod, ModKey::createNXYZ(ModId::Position, id));
        break;
    case hash("width"):
        width = opcode.read(Default::width);
        break;
    case_any_ccN("width"):
        processGenericCc(opcode, Default::widthMod, ModKey::createNXYZ(ModId::Width, id));
        break;
    case hash("amp_keycenter"):
        ampKeycenter = opcode.read(Default::key);
        break;
    case hash("amp_keytrack"):
        ampKeytrack = opcode.read(Default::ampKeytrack);
        break;
    case hash("amp_veltrack"):
        ampVeltrack = opcode.read(Default::ampVeltrack);
        break;
    case hash("amp_random"):
        ampRandom = opcode.read(Default::ampRandom);
        break;
    case hash("amp_velcurve_&"):
        {
            if (opcode.parameters.back() > 127)
                return false;

            const auto inputVelocity = static_cast<uint8_t>(opcode.parameters.back());
            velocityPoints.emplace_back(inputVelocity, opcode.read(Default::ampVelcurve));
        }
        break;
    case hash("xfin_lokey"):
        crossfadeKeyInRange.setStart(opcode.read(Default::loKey));
        break;
    case hash("xfin_hikey"):
        crossfadeKeyInRange.setEnd(opcode.read(Default::loKey)); // loKey for the proper default
        break;
    case hash("xfout_lokey"):
        crossfadeKeyOutRange.setStart(opcode.read(Default::hiKey)); // hiKey for the proper default
        break;
    case hash("xfout_hikey"):
        crossfadeKeyOutRange.setEnd(opcode.read(Default::hiKey));
        break;
    case hash("xfin_lovel"):
        crossfadeVelInRange.setStart(opcode.read(Default::loVel));
        break;
    case hash("xfin_hivel"):
        crossfadeVelInRange.setEnd(opcode.read(Default::loVel)); // loVel for the proper default
        break;
    case hash("xfout_lovel"):
        crossfadeVelOutRange.setStart(opcode.read(Default::hiVel)); // hiVel for the proper default
        break;
    case hash("xfout_hivel"):
        crossfadeVelOutRange.setEnd(opcode.read(Default::hiVel));
        break;
    case hash("xf_keycurve"):
        crossfadeKeyCurve = opcode.read(Default::crossfadeCurve);
        break;
    case hash("xf_velcurve"):
        crossfadeVelCurve = opcode.read(Default::crossfadeCurve);
        break;
    case hash("xfin_locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        crossfadeCCInRange[opcode.parameters.back()].setStart(
            opcode.read(Default::loCC)
        );
        break;
    case hash("xfin_hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        crossfadeCCInRange[opcode.parameters.back()].setEnd(
            opcode.read(Default::loCC) // loCC for the proper default
        );
        break;
    case hash("xfout_locc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        crossfadeCCOutRange[opcode.parameters.back()].setStart(
            opcode.read(Default::hiCC) // hiCC for the proper default
        );
        break;
    case hash("xfout_hicc&"):
        if (opcode.parameters.back() >= config::numCCs)
            return false;
        crossfadeCCOutRange[opcode.parameters.back()].setEnd(
            opcode.read(Default::hiCC)
        );
        break;
    case hash("xf_cccurve"):
        crossfadeCCCurve = opcode.read(Default::crossfadeCurve);
        break;
    case hash("rt_decay"):
        rtDecay = opcode.read(Default::rtDecay);
        break;
    case hash("global_amplitude"):
        globalAmplitude = opcode.read(Default::amplitude);
        break;
    case hash("master_amplitude"):
        masterAmplitude = opcode.read(Default::amplitude);
        break;
    case hash("group_amplitude"):
        groupAmplitude = opcode.read(Default::amplitude);
        break;
    case hash("global_volume"):
        globalVolume = opcode.read(Default::volume);
        break;
    case hash("master_volume"):
        masterVolume = opcode.read(Default::volume);
        break;
    case hash("group_volume"):
        groupVolume = opcode.read(Default::volume);
        break;

    // Performance parameters: filters
    case hash("cutoff&"): // also cutoff
        {
            const auto filterIndex = opcode.parameters.empty() ? 0 : (opcode.parameters.back() - 1);
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].cutoff = opcode.read(Default::filterCutoff);
        }
        break;
    case hash("resonance&"): // also resonance
        {
            const auto filterIndex = opcode.parameters.empty() ? 0 : (opcode.parameters.back() - 1);
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].resonance = opcode.read(Default::filterResonance);
        }
        break;
    case_any_ccN("cutoff&"): // also cutoff_oncc&, cutoff_cc&, cutoff&_cc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterCutoffMod, ModKey::createNXYZ(ModId::FilCutoff, id, filterIndex));
        }
        break;
    case_any_ccN("resonance&"): // also resonance_oncc&, resonance_cc&, resonance&_cc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterResonanceMod, ModKey::createNXYZ(ModId::FilResonance, id, filterIndex));
        }
        break;
    case hash("cutoff&_chanaft"):
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            const ModKey source = ModKey::createNXYZ(ModId::ChannelAftertouch);
            const ModKey target = ModKey::createNXYZ(ModId::FilCutoff, id, filterIndex);
            getOrCreateConnection(source, target).sourceDepth = opcode.read(Default::filterCutoffMod);
        }
        break;
    case hash("fil&_keytrack"): // also fil_keytrack
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].keytrack = opcode.read(Default::filterKeytrack);
        }
        break;
    case hash("fil&_keycenter"): // also fil_keycenter
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].keycenter = opcode.read(Default::key);
        }
        break;
    case hash("fil&_veltrack"): // also fil_veltrack
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].veltrack = opcode.read(Default::filterVeltrack);
        }
        break;
    case hash("fil&_random"): // also fil_random, cutoff_random, cutoff&_random
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].random = opcode.read(Default::filterRandom);
        }
        break;
    case hash("fil&_gain"): // also fil_gain
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;
            filters[filterIndex].gain = opcode.read(Default::filterGain);
        }
        break;
    case_any_ccN("fil&_gain"): // also fil_gain_oncc&
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            processGenericCc(opcode, Default::filterGainMod, ModKey::createNXYZ(ModId::FilGain, id, filterIndex));
        }
        break;
    case hash("fil&_type"): // also fil_type, filtype
        {
            const auto filterIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(filters, filterIndex + 1, Default::numFilters))
                return false;

            filters[filterIndex].type = opcode.read(Default::filter);
        }
        break;

    // Performance parameters: EQ
    case hash("eq&_bw"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            equalizers[eqIndex].bandwidth = opcode.read(Default::eqBandwidth);
        }
        break;
    case_any_ccN("eq&_bw"): // also eq&_bwcc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqBandwidthMod, ModKey::createNXYZ(ModId::EqBandwidth, id, eqIndex));
        }
        break;
    case hash("eq&_freq"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            equalizers[eqIndex].frequency = opcode.read(Default::eqFrequency);
        }
        break;
    case_any_ccN("eq&_freq"): // also eq&_freqcc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqFrequencyMod, ModKey::createNXYZ(ModId::EqFrequency, id, eqIndex));
        }
        break;
    case hash("eq&_veltofreq"): // also eq&_vel2freq
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            equalizers[eqIndex].vel2frequency = opcode.read(Default::eqVel2Frequency);
        }
        break;
    case hash("eq&_gain"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            equalizers[eqIndex].gain = opcode.read(Default::eqGain);
        }
        break;
    case_any_ccN("eq&_gain"): // also eq&_gaincc&
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            processGenericCc(opcode, Default::eqGainMod, ModKey::createNXYZ(ModId::EqGain, id, eqIndex));
        }
        break;
    case hash("eq&_veltogain"): // also eq&_vel2gain
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;
            equalizers[eqIndex].vel2gain = opcode.read(Default::eqVel2Gain);
        }
        break;
    case hash("eq&_type"):
        {
            const auto eqIndex = opcode.parameters.front() - 1;
            if (!extendIfNecessary(equalizers, eqIndex + 1, Default::numEQs))
                return false;

            equalizers[eqIndex].type =
                opcode.read(Default::eq);

       }
        break;

    // Performance parameters: pitch
    case hash("pitch_keycenter"):
        if (opcode.value == "sample")
            pitchKeycenterFromSample = true;
        else {
            pitchKeycenterFromSample = false;
            pitchKeycenter = opcode.read(Default::key);
        }
        break;
    case hash("pitch_keytrack"):
        pitchKeytrack = opcode.read(Default::pitchKeytrack);
        break;
    case hash("pitch_veltrack"):
        pitchVeltrack = opcode.read(Default::pitchVeltrack);
        break;
    case hash("pitch_random"):
        pitchRandom = opcode.read(Default::pitchRandom);
        break;
    case hash("transpose"):
        transpose = opcode.read(Default::transpose);
        break;
    case hash("pitch"): // also tune
        pitch = opcode.read(Default::pitch);
        break;
    case_any_ccN("pitch"): // also tune
        processGenericCc(opcode, Default::pitchMod, ModKey::createNXYZ(ModId::Pitch, id));
        break;
    case hash("bend_up"): // also bendup
        bendUp = opcode.read(Default::bendUp);
        break;
    case hash("bend_down"): // also benddown
        bendDown = opcode.read(Default::bendDown);
        break;
    case hash("bend_step"):
        bendStep = opcode.read(Default::bendStep);
        break;
    case hash("bend_smooth"):
        bendSmooth = opcode.read(Default::smoothCC);
        break;

    case hash("effect&"):
    {
        const auto effectNumber = opcode.parameters.back();
        if (!effectNumber || effectNumber < 1 || effectNumber > config::maxEffectBuses)
            break;
        if (static_cast<size_t>(effectNumber + 1) > gainToEffect.size())
            gainToEffect.resize(effectNumber + 1);
        gainToEffect[effectNumber] = opcode.read(Default::effect);
        break;
    }
    case hash("sw_default"):
        defaultSwitch = opcode.read(Default::key);
        break;

    // Ignored opcodes
    case hash("hichan"):
    case hash("lochan"):
    case hash("ampeg_depth"):
    case hash("ampeg_veltodepth"): // also ampeg_vel2depth
        break;

    default: {
        // Amplitude Envelope
        if (absl::StartsWith(opcode.name, "ampeg_")) {
            if (parseEGOpcode(opcode, amplitudeEG))
                return true;
        }
        // Pitch Envelope
        if (absl::StartsWith(opcode.name, "pitcheg_")) {
            if (parseEGOpcode(opcode, pitchEG)) {
                getOrCreateConnection(
                    ModKey::createNXYZ(ModId::PitchEG, id),
                    ModKey::createNXYZ(ModId::Pitch, id));
                return true;
            }
        }
        // Filter Envelope
        if (absl::StartsWith(opcode.name, "fileg_")) {
            if (parseEGOpcode(opcode, filterEG)) {
                getOrCreateConnection(
                    ModKey::createNXYZ(ModId::FilEG, id),
                    ModKey::createNXYZ(ModId::FilCutoff, id));
                return true;
            }
        }

        // Amplitude LFO
        if (absl::StartsWith(opcode.name, "amplfo_")) {
            if (parseLFOOpcode(opcode, amplitudeLFO)) {
                getOrCreateConnection(
                    ModKey::createNXYZ(ModId::AmpLFO, id),
                    ModKey::createNXYZ(ModId::Volume, id));
                return true;
            }
        }
        // Pitch LFO
        if (absl::StartsWith(opcode.name, "pitchlfo_")) {
            if (parseLFOOpcode(opcode, pitchLFO)) {
                getOrCreateConnection(
                    ModKey::createNXYZ(ModId::PitchLFO, id),
                    ModKey::createNXYZ(ModId::Pitch, id));
                return true;
            }
        }
        // Filter LFO
        if (absl::StartsWith(opcode.name, "fillfo_")) {
            if (parseLFOOpcode(opcode, filterLFO)) {
                getOrCreateConnection(
                    ModKey::createNXYZ(ModId::FilLFO, id),
                    ModKey::createNXYZ(ModId::FilCutoff, id));
                return true;
            }
        }

        //
        const std::string letterOnlyName = opcode.getLetterOnlyName();

        // Modulation: LFO
        if (absl::StartsWith(letterOnlyName, "lfo&_")) {
            if (parseLFOOpcodeV2(opcode))
                return true;
        }
        // Modulation: Flex EG
        if (absl::StartsWith(letterOnlyName, "eg&_")) {
            if (parseEGOpcodeV2(opcode))
                return true;
        }

        return false;
    }

    }

    return true;
}

bool sfz::Region::parseLFOOpcode(const Opcode& opcode, LFODescription& lfo)
{
    #define case_any_lfo(param)                     \
        case hash("amplfo_" param):                 \
        case hash("pitchlfo_" param):               \
        case hash("fillfo_" param)                  \

    #define case_any_lfo_any_ccN(param)             \
        case_any_ccN("amplfo_" param):              \
        case_any_ccN("pitchlfo_" param):            \
        case_any_ccN("fillfo_" param)               \

    //
    ModKey sourceKey;
    ModKey sourceDepthKey;
    ModKey targetKey;
    OpcodeSpec<float> depthSpec;
    OpcodeSpec<float> depthModSpec;

    if (absl::StartsWith(opcode.name, "amplfo_")) {
        sourceKey = ModKey::createNXYZ(ModId::AmpLFO, id);
        sourceDepthKey = ModKey::createNXYZ(ModId::AmpLFODepth, id);
        targetKey = ModKey::createNXYZ(ModId::Volume, id);
        lfo.freqKey = ModKey::createNXYZ(ModId::AmpLFOFrequency, id);
        depthSpec = Default::ampLFODepth;
        depthModSpec = Default::volumeMod;
    }
    else if (absl::StartsWith(opcode.name, "pitchlfo_")) {
        sourceKey = ModKey::createNXYZ(ModId::PitchLFO, id);
        sourceDepthKey = ModKey::createNXYZ(ModId::PitchLFODepth, id);
        targetKey = ModKey::createNXYZ(ModId::Pitch, id);
        lfo.freqKey = ModKey::createNXYZ(ModId::PitchLFOFrequency, id);
        depthSpec = Default::pitchLFODepth;
        depthModSpec = Default::pitchMod;
    }
    else if (absl::StartsWith(opcode.name, "fillfo_")) {
        sourceKey = ModKey::createNXYZ(ModId::FilLFO, id);
        sourceDepthKey = ModKey::createNXYZ(ModId::FilLFODepth, id);
        targetKey = ModKey::createNXYZ(ModId::FilCutoff, id);
        lfo.freqKey = ModKey::createNXYZ(ModId::FilLFOFrequency, id);
        depthSpec = Default::filLFODepth;
        depthModSpec = Default::filterCutoffMod;
    }
    else {
        ASSERTFALSE;
        return false;
    }

    //
    switch (opcode.lettersOnlyHash) {

    case_any_lfo("delay"):
        lfo.delay = opcode.read(Default::lfoDelay);
        break;
    case_any_lfo("depth"):
        getOrCreateConnection(sourceKey, targetKey).sourceDepth = opcode.read(depthSpec);
        break;
    case_any_lfo_any_ccN("depth"): // also depthcc&
        getOrCreateConnection(sourceKey, targetKey).sourceDepthMod = sourceDepthKey;
        processGenericCc(opcode, depthModSpec, sourceDepthKey);
        break;
    case_any_lfo("depthchanaft"):
        // TODO(jpc) LFO v1
        break;
    case_any_lfo("depthpolyaft"):
        // TODO(jpc) LFO v1
        break;
    case_any_lfo("fade"):
        lfo.fade = opcode.read(Default::lfoFade);
        break;
    case_any_lfo("freq"):
        lfo.freq = opcode.read(Default::lfoFreq);
        break;
    case_any_lfo_any_ccN("freq"): // also freqcc&
        processGenericCc(opcode, Default::lfoFreqMod, lfo.freqKey);
        break;
    case_any_lfo("freqchanaft"):
        // TODO(jpc) LFO v1
        break;
    case_any_lfo("freqpolyaft"):
        // TODO(jpc) LFO v1
        break;

    // sfizz extension
    case_any_lfo("wave"):
        lfo.sub[0].wave = opcode.read(Default::lfoWave);
        break;

    default:
        return false;
    }

    #undef case_any_lfo

    return true;
}

bool sfz::Region::parseLFOOpcode(const Opcode& opcode, absl::optional<LFODescription>& lfo)
{
    bool create = lfo == absl::nullopt;
    if (create) {
        lfo = LFODescription();
        lfo->sub[0].wave = LFOWave::Sine; // the LFO v1 default
    }

    bool parsed = parseLFOOpcode(opcode, *lfo);
    if (!parsed && create)
        lfo = absl::nullopt;

    return parsed;
}

bool sfz::Region::parseEGOpcode(const Opcode& opcode, EGDescription& eg)
{
    #define case_any_eg(param)                      \
        case hash("ampeg_" param):                  \
        case hash("pitcheg_" param):                \
        case hash("fileg_" param)                   \

    switch (opcode.lettersOnlyHash) {
    case_any_eg("attack"):
        eg.attack = opcode.read(Default::egTime);
        break;
    case_any_eg("decay"):
        eg.decay = opcode.read(Default::egTime);
        break;
    case_any_eg("delay"):
        eg.delay = opcode.read(Default::egTime);
        break;
    case_any_eg("hold"):
        eg.hold = opcode.read(Default::egTime);
        break;
    case_any_eg("release"):
        eg.release = opcode.read(Default::egRelease);
        break;
    case_any_eg("start"):
        eg.start = opcode.read(Default::egPercent);
        break;
    case_any_eg("sustain"):
        eg.sustain = opcode.read(Default::egPercent);
        break;
    case_any_eg("veltoattack"): // also vel2attack
        eg.vel2attack = opcode.read(Default::egTimeMod);
        break;
    case_any_eg("veltodecay"): // also vel2decay
        eg.vel2decay = opcode.read(Default::egTimeMod);
        break;
    case_any_eg("veltodelay"): // also vel2delay
        eg.vel2delay = opcode.read(Default::egTimeMod);
        break;
    case_any_eg("veltohold"): // also vel2hold
        eg.vel2hold = opcode.read(Default::egTimeMod);
        break;
    case_any_eg("veltorelease"): // also vel2release
        eg.vel2release = opcode.read(Default::egTimeMod);
        break;
    case_any_eg("veltosustain"): // also vel2sustain
        eg.vel2sustain = opcode.read(Default::egPercentMod);
        break;
    case_any_eg("attack_oncc&"): // also attackcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccAttack[opcode.parameters.back()] = opcode.read(Default::egTimeMod);

        break;
    case_any_eg("decay_oncc&"): // also decaycc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccDecay[opcode.parameters.back()] = opcode.read(Default::egTimeMod);

        break;
    case_any_eg("delay_oncc&"): // also delaycc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccDelay[opcode.parameters.back()] = opcode.read(Default::egTimeMod);

        break;
    case_any_eg("hold_oncc&"): // also holdcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccHold[opcode.parameters.back()] = opcode.read(Default::egTimeMod);

        break;
    case_any_eg("release_oncc&"): // also releasecc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccRelease[opcode.parameters.back()] = opcode.read(Default::egTimeMod);

        break;
    case_any_eg("start_oncc&"): // also startcc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccStart[opcode.parameters.back()] = opcode.read(Default::egPercentMod);

        break;
    case_any_eg("sustain_oncc&"): // also sustaincc&
        if (opcode.parameters.back() >= config::numCCs)
            return false;

        eg.ccSustain[opcode.parameters.back()] = opcode.read(Default::egPercentMod);

        break;

    case hash("pitcheg_depth"):
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::PitchEG, id),
            ModKey::createNXYZ(ModId::Pitch, id)).sourceDepth = opcode.read(Default::egDepth);
        break;
    case hash("fileg_depth"):
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::FilEG, id),
            ModKey::createNXYZ(ModId::FilCutoff, id)).sourceDepth = opcode.read(Default::egDepth);
        break;

    case hash("pitcheg_veltodepth"): // also pitcheg_vel2depth
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::PitchEG, id),
            ModKey::createNXYZ(ModId::Pitch, id)).velToDepth = opcode.read(Default::egVel2Depth);
        break;
    case hash("fileg_veltodepth"): // also fileg_vel2depth
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::FilEG, id),
            ModKey::createNXYZ(ModId::FilCutoff, id)).velToDepth = opcode.read(Default::egVel2Depth);
        break;

    case_any_ccN("pitcheg_depth"):
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::PitchEG, id),
            ModKey::createNXYZ(ModId::Pitch, id)).sourceDepthMod = ModKey::createNXYZ(ModId::PitchEGDepth, id);
        processGenericCc(opcode, Default::pitchMod, ModKey::createNXYZ(ModId::PitchEGDepth, id));
        break;
    case_any_ccN("fileg_depth"):
        getOrCreateConnection(
            ModKey::createNXYZ(ModId::FilEG, id),
            ModKey::createNXYZ(ModId::FilCutoff, id)).sourceDepthMod = ModKey::createNXYZ(ModId::FilEGDepth, id);
        processGenericCc(opcode, Default::filterCutoffMod, ModKey::createNXYZ(ModId::FilEGDepth, id));
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

bool sfz::Region::parseLFOOpcodeV2(const Opcode& opcode)
{
    const unsigned lfoNumber1Based = opcode.parameters.front();

    if (lfoNumber1Based <= 0)
        return false;
    if (!extendIfNecessary(lfos, lfoNumber1Based, Default::numLFOs))
        return false;

    const unsigned lfoNumber = lfoNumber1Based - 1;
    LFODescription& lfo = lfos[lfoNumber];

    //
    lfo.beatsKey = ModKey::createNXYZ(ModId::LFOBeats, id, lfoNumber);
    lfo.freqKey = ModKey::createNXYZ(ModId::LFOFrequency, id, lfoNumber);

    //
    auto getOrCreateLFOStep = [&opcode, &lfo]() -> float* {
        const unsigned stepNumber1Based = opcode.parameters[1];
        if (stepNumber1Based <= 0 || stepNumber1Based > config::maxLFOSteps)
            return nullptr;
        if (!lfo.seq)
            lfo.seq = LFODescription::StepSequence();
        if (!extendIfNecessary(lfo.seq->steps, stepNumber1Based, Default::numLFOSteps))
            return nullptr;
        return &lfo.seq->steps[stepNumber1Based - 1];
    };
    auto getOrCreateLFOSub = [&opcode, &lfo]() -> LFODescription::Sub* {
        const unsigned subNumber1Based = opcode.parameters[1];
        if (subNumber1Based <= 0 || subNumber1Based > config::maxLFOSubs)
            return nullptr;
        if (!extendIfNecessary(lfo.sub, subNumber1Based, Default::numLFOSubs))
            return nullptr;
        return &lfo.sub[subNumber1Based - 1];
    };
    auto LFO_EG_filter_EQ_target = [this, &opcode, lfoNumber](ModId sourceId, ModId targetId, const OpcodeSpec<float>& spec) -> bool {
        const unsigned index = opcode.parameters.size() == 2 ? opcode.parameters.back() - 1 : 0;
        if (!extendIfNecessary(filters, index + 1, Default::numFilters))
            return false;
        const ModKey source = ModKey::createNXYZ(sourceId, id, lfoNumber);
        const ModKey target = ModKey::createNXYZ(targetId, id, index);
        getOrCreateConnection(source, target).sourceDepth = opcode.read(spec);
        return true;
    };

    //
    switch (opcode.lettersOnlyHash) {

    // Modulation: LFO
    case hash("lfo&_freq"):
        lfo.freq = opcode.read(Default::lfoFreq);
        break;
    case_any_ccN("lfo&_freq"):
        processGenericCc(opcode, Default::lfoFreqMod, ModKey::createNXYZ(ModId::LFOFrequency, id, lfoNumber));
        break;
    case hash("lfo&_beats"):
        lfo.beats = opcode.read(Default::lfoBeats);
        break;
    case_any_ccN("lfo&_beats"):
        processGenericCc(opcode, Default::lfoBeatsMod, ModKey::createNXYZ(ModId::LFOBeats, id, lfoNumber));
        break;
    case hash("lfo&_phase"):
        lfo.phase0 = opcode.read(Default::lfoPhase);
        break;
    case hash("lfo&_delay"):
        lfo.delay = opcode.read(Default::lfoDelay);
        break;
    case hash("lfo&_fade"):
        lfo.fade = opcode.read(Default::lfoFade);
        break;
    case hash("lfo&_count"):
        lfo.count = opcode.read(Default::lfoCount);
        break;
    case hash("lfo&_steps"):
        if (!lfo.seq)
            lfo.seq = LFODescription::StepSequence();
        lfo.seq->steps.resize(opcode.read(Default::lfoSteps));
        break;
    case hash("lfo&_step&"):
        if (float* step = getOrCreateLFOStep())
            *step = opcode.read(Default::lfoStepX);
        else
            return false;
        break;
    case hash("lfo&_wave&"): // also lfo&_wave
        if (LFODescription::Sub* sub = getOrCreateLFOSub())
            sub->wave = opcode.read(Default::lfoWave);
        else
            return false;
        break;
    case hash("lfo&_offset&"): // also lfo&_offset
        if (LFODescription::Sub* sub = getOrCreateLFOSub())
            sub->offset = opcode.read(Default::lfoOffset);
        else
            return false;
        break;
    case hash("lfo&_ratio&"): // also lfo&_ratio
        if (LFODescription::Sub* sub = getOrCreateLFOSub())
            sub->ratio = opcode.read(Default::lfoRatio);
        else
            return false;
        break;
    case hash("lfo&_scale&"): // also lfo&_scale
        if (LFODescription::Sub* sub = getOrCreateLFOSub())
            sub->scale = opcode.read(Default::lfoScale);
        else
            return false;
        break;

    // Modulation: LFO (targets)
    case hash("lfo&_amplitude"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::amplitudeMod);
        }
        break;
    case hash("lfo&_pan"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::panMod);
        }
        break;
    case hash("lfo&_width"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Width, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::widthMod);
        }
        break;
    case hash("lfo&_position"): // sfizz extension
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Position, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::positionMod);
        }
        break;
    case hash("lfo&_pitch"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::pitchMod);
        }
        break;
    case hash("lfo&_volume"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::volumeMod);
        }
        break;
    case_any_ccN("lfo&_volume"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
            const ModKey depthModifier = ModKey::createNXYZ(ModId::LFOVolumeDepth, id);
            getOrCreateConnection(source, target).sourceDepthMod = depthModifier;
            processGenericCc(opcode, Default::volumeMod, depthModifier);
        }
        break;
    case_any_ccN("lfo&_pitch"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
            const ModKey depthModifier = ModKey::createNXYZ(ModId::LFOPitchDepth, id);
            getOrCreateConnection(source, target).sourceDepthMod = depthModifier;
            processGenericCc(opcode, Default::pitchMod, depthModifier);
        }
        break;
    case_any_ccN("lfo&_cutoff"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber);
            const ModKey target = ModKey::createNXYZ(ModId::FilCutoff, id);
            const ModKey depthModifier = ModKey::createNXYZ(ModId::LFOFilCutoffDepth, id);
            getOrCreateConnection(source, target).sourceDepthMod = depthModifier;
            processGenericCc(opcode, Default::filterCutoffMod, depthModifier);
        }
        break;

    case hash("lfo&_cutoff&"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilCutoff, Default::filterCutoffMod);
        break;
    case hash("lfo&_resonance&"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilResonance, Default::filterResonanceMod);
        break;
    case hash("lfo&_fil&gain"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::FilGain, Default::filterGainMod);
        break;
    case hash("lfo&_eq&gain"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqGain, Default::eqGainMod);
        break;
    case hash("lfo&_eq&freq"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqFrequency, Default::eqFrequencyMod);
        break;
    case hash("lfo&_eq&bw"):
        LFO_EG_filter_EQ_target(ModId::LFO, ModId::EqBandwidth, Default::eqBandwidthMod);
        break;

    default:
        return false;
    }

    return true;
}

bool sfz::Region::parseEGOpcodeV2(const Opcode& opcode)
{
    const unsigned egNumber1Based = opcode.parameters.front();
    if (egNumber1Based <= 0)
        return false;
    if (!extendIfNecessary(flexEGs, egNumber1Based, Default::numFlexEGs))
        return false;

    const unsigned egNumber = egNumber1Based - 1;
    FlexEGDescription& eg = flexEGs[egNumber];

    //
    auto getOrCreateEGPoint = [&opcode, &eg]() -> FlexEGPoint* {
        const auto pointNumber = opcode.parameters[1];
        if (!extendIfNecessary(eg.points, pointNumber + 1, Default::numFlexEGPoints))
            return nullptr;
        return &eg.points[pointNumber];
    };
    auto LFO_EG_filter_EQ_target = [this, &opcode, egNumber](ModId sourceId, ModId targetId, const OpcodeSpec<float>& spec) -> bool {
        const unsigned index = opcode.parameters.size() == 2 ? opcode.parameters.back() - 1 : 0;
        if (!extendIfNecessary(filters, index + 1, Default::numFilters))
            return false;
        const ModKey source = ModKey::createNXYZ(sourceId, id, egNumber);
        const ModKey target = ModKey::createNXYZ(targetId, id, index);
        getOrCreateConnection(source, target).sourceDepth = opcode.read(spec);
        return true;
    };

    //
    switch (opcode.lettersOnlyHash) {

    // Flex envelopes
    case hash("eg&_dynamic"):
        eg.dynamic = opcode.read(Default::flexEGDynamic);
        break;
    case hash("eg&_sustain"):
        eg.sustain = opcode.read(Default::flexEGSustain);
        break;
    case hash("eg&_time&"):
        if (FlexEGPoint* point = getOrCreateEGPoint())
            point->time = opcode.read(Default::flexEGPointTime);
        else
            return false;
        break;
    case hash("eg&_level&"):
        if (FlexEGPoint* point = getOrCreateEGPoint())
            point->level = opcode.read(Default::flexEGPointLevel);
        else
            return false;
        break;
    case hash("eg&_shape&"):
        if (FlexEGPoint* point = getOrCreateEGPoint())
            point->setShape(opcode.read(Default::flexEGPointShape));
        else
            return false;
        break;

    // Modulation: Flex EG (targets)
    case hash("eg&_amplitude"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::amplitudeMod);
        }
        break;
    case hash("eg&_pan"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::panMod);
        }
        break;
    case hash("eg&_width"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Width, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::widthMod);
        }
        break;
    case hash("eg&_position"): // sfizz extension
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Position, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::positionMod);
        }
        break;
    case hash("eg&_pitch"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::pitchMod);
        }
        break;
    case hash("eg&_volume"):
        {
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber);
            const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::volumeMod);
        }
        break;
    case hash("eg&_cutoff&"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilCutoff, Default::filterCutoffMod);
        break;
    case hash("eg&_resonance&"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilResonance, Default::filterResonanceMod);
        break;
    case hash("eg&_fil&gain"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::FilGain, Default::filterGainMod);
        break;
    case hash("eg&_eq&gain"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqGain, Default::eqGainMod);
        break;
    case hash("eg&_eq&freq"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqFrequency, Default::eqFrequencyMod);
        break;
    case hash("eg&_eq&bw"):
        LFO_EG_filter_EQ_target(ModId::Envelope, ModId::EqBandwidth, Default::eqBandwidthMod);
        break;

    case hash("eg&_ampeg"):
    {
        auto ampeg = opcode.read(Default::flexEGAmpeg);
        if (eg.ampeg != ampeg) {
            eg.ampeg = ampeg;
            flexAmpEG = absl::nullopt;
            for (size_t i = 0, n = flexEGs.size(); i < n && !flexAmpEG; ++i) {
                if (flexEGs[i].ampeg)
                    flexAmpEG = static_cast<uint8_t>(i);
            }
        }
        break;
    }

    default:
        return false;
    }

    return true;
}

bool sfz::Region::processGenericCc(const Opcode& opcode, OpcodeSpec<float> spec, const ModKey& target)
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
            conn->sourceDepth = opcode.read(spec);
            break;
        case kOpcodeCurveCcN:
                p.curve = opcode.read(Default::curveCC);
            break;
        case kOpcodeStepCcN:
            {
                const float maxStep =
                    max(std::abs(spec.bounds.getStart()), std::abs(spec.bounds.getEnd()));
                const OpcodeSpec<float> stepCC { 0.0f, Range<float>(0.0f, maxStep), 0 };
                p.step = opcode.read(stepCC);
            }
            break;
        case kOpcodeSmoothCcN:
            p.smooth = opcode.read(Default::smoothCC);
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

void sfz::Region::delaySustainRelease(int noteNumber, float velocity) noexcept
{
    if (delayedSustainReleases.size() == delayedSustainReleases.capacity())
        return;

    delayedSustainReleases.emplace_back(noteNumber, velocity);
}

void sfz::Region::delaySostenutoRelease(int noteNumber, float velocity) noexcept
{
    if (delayedSostenutoReleases.size() == delayedSostenutoReleases.capacity())
        return;

    delayedSostenutoReleases.emplace_back(noteNumber, velocity);
}

void sfz::Region::removeFromSostenutoReleases(int noteNumber) noexcept
{
    swapAndPopFirst(delayedSostenutoReleases, [=](const std::pair<int, float>& p) {
        return p.first == noteNumber;
    });
}

void sfz::Region::storeSostenutoNotes() noexcept
{
    ASSERT(delayedSostenutoReleases.empty());
    for (int note = keyRange.getStart(); note <= keyRange.getEnd(); ++note) {
        if (midiState.isNotePressed(note))
            delaySostenutoRelease(note, midiState.getNoteVelocity(note));
    }
}


bool sfz::Region::isNoteSustained(int noteNumber) const noexcept
{
    return absl::c_find_if(delayedSustainReleases, [=](const std::pair<int, float>& p) {
        return p.first == noteNumber;
    }) != delayedSustainReleases.end();
}

bool sfz::Region::isNoteSostenutoed(int noteNumber) const noexcept
{
    return absl::c_find_if(delayedSostenutoReleases, [=](const std::pair<int, float>& p) {
        return p.first == noteNumber;
    }) != delayedSostenutoReleases.end();
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

    if (velocityOverride == VelocityOverride::previous)
        velocity = midiState.getLastVelocity();

    const bool velOk = velocityRange.containsWithEnd(velocity);
    const bool randOk = randRange.contains(randValue) || (randValue >= 1.0f && randRange.isValid() && randRange.getEnd() >= 1.0f);
    const bool firstLegatoNote = (trigger == Trigger::first && midiState.getActiveNotes() == 1);
    const bool attackTrigger = (trigger == Trigger::attack);
    const bool notFirstLegatoNote = (trigger == Trigger::legato && midiState.getActiveNotes() > 1);

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
    const bool randOk = randRange.contains(randValue) || (randValue >= 1.0f && randRange.isValid() && randRange.getEnd() >= 1.0f);

    if (!(velOk && keyOk && randOk))
        return false;

    // Release logic

    if (trigger == Trigger::release_key)
        return true;

    if (trigger == Trigger::release) {
        const bool sostenutoed = isNoteSostenutoed(noteNumber);

        if (sostenutoed && !sostenutoPressed) {
            removeFromSostenutoReleases(noteNumber);
            if (sustainPressed)
                delaySustainRelease(noteNumber, midiState.getNoteVelocity(noteNumber));
        }

        if (!sostenutoPressed || !sostenutoed) {
            if (sustainPressed)
                delaySustainRelease(noteNumber, midiState.getNoteVelocity(noteNumber));
            else
                return true;
        }
    }

    return false;
}

bool sfz::Region::registerCC(int ccNumber, float ccValue) noexcept
{
    ASSERT(ccValue >= 0.0f && ccValue <= 1.0f);

    if (ccNumber == sustainCC)
        sustainPressed = checkSustain && ccValue >= sustainThreshold;

    if (ccNumber == sostenutoCC) {
        const bool newState = checkSostenuto && ccValue >= sostenutoThreshold;
        if (!sostenutoPressed && newState)
            storeSostenutoNotes();

        if (!newState && sostenutoPressed)
            delayedSostenutoReleases.clear();

        sostenutoPressed = newState;
    }

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

void sfz::Region::registerAftertouch(float aftertouch) noexcept
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

    fast_real_distribution<float> pitchDistribution { 0.0f, pitchRandom };
    float pitchVariationInCents = pitchKeytrack * (noteNumber - pitchKeycenter); // note difference with pitch center
    pitchVariationInCents += pitch; // sample tuning
    pitchVariationInCents += config::centPerSemitone * transpose; // sample transpose
    pitchVariationInCents += velocity * pitchVeltrack; // track velocity
    pitchVariationInCents += pitchDistribution(Random::randomGenerator); // random pitch changes
    return centsFactor(pitchVariationInCents);
}

float sfz::Region::getBaseVolumedB(int noteNumber) const noexcept
{
    fast_real_distribution<float> volumeDistribution { 0.0f, ampRandom };
    auto baseVolumedB = volume + volumeDistribution(Random::randomGenerator);
    baseVolumedB += globalVolume;
    baseVolumedB += masterVolume;
    baseVolumedB += groupVolume;
    if (trigger == Trigger::release || trigger == Trigger::release_key)
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
    return Default::offset.bounds.clamp(finalOffset) * static_cast<uint64_t>(factor);
}

float sfz::Region::getDelay() const noexcept
{
    fast_real_distribution<float> delayDistribution { 0, delayRandom };
    float finalDelay { delay };
    finalDelay += delayDistribution(Random::randomGenerator);
    for (const auto& mod: delayCC)
        finalDelay += mod.data * midiState.getCCValue(mod.cc);
    return Default::delay.bounds.clamp(finalDelay);
}

uint32_t sfz::Region::trueSampleEnd(Oversampling factor) const noexcept
{
    if (sampleEnd <= 0)
        return 0;

    return static_cast<uint32_t>(sampleEnd) * static_cast<uint32_t>(factor);
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
    if (keyRange != Default::key.bounds) {
        const auto start = keyRange.getStart();
        const auto end = keyRange.getEnd();
        keyRange.setStart(offsetAndClampKey(start, offset));
        keyRange.setEnd(offsetAndClampKey(end, offset));
    }
    pitchKeycenter = offsetAndClampKey(pitchKeycenter, offset);

    // Offset key switches
    if (upKeyswitch)
        upKeyswitch = offsetAndClampKey(*upKeyswitch, offset);
    if (lastKeyswitch)
        lastKeyswitch = offsetAndClampKey(*lastKeyswitch, offset);
    if (downKeyswitch)
        downKeyswitch = offsetAndClampKey(*downKeyswitch, offset);
    if (previousKeyswitch)
        previousKeyswitch = offsetAndClampKey(*previousKeyswitch, offset);

    // Offset crossfade ranges
    if (crossfadeKeyInRange != Default::crossfadeKeyInRange) {
        const auto start = crossfadeKeyInRange.getStart();
        const auto end = crossfadeKeyInRange.getEnd();
        crossfadeKeyInRange.setStart(offsetAndClampKey(start, offset));
        crossfadeKeyInRange.setEnd(offsetAndClampKey(end, offset));
    }

    if (crossfadeKeyOutRange != Default::crossfadeKeyOutRange) {
        const auto start = crossfadeKeyOutRange.getStart();
        const auto end = crossfadeKeyOutRange.getEnd();
        crossfadeKeyOutRange.setStart(offsetAndClampKey(start, offset));
        crossfadeKeyOutRange.setEnd(offsetAndClampKey(end, offset));
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

sfz::Region::Connection* sfz::Region::getConnectionFromCC(int sourceCC, const ModKey& target)
{
    for (sfz::Region::Connection& conn : connections) {
        if (conn.source.id() == sfz::ModId::Controller && conn.target == target) {
            auto p = conn.source.parameters();
            if (p.cc == sourceCC)
                return &conn;
        }
    }
    return nullptr;
}

bool sfz::Region::disabled() const noexcept
{
    return (sampleEnd == 0);
}

absl::optional<float> sfz::Region::ccModDepth(int cc, ModId id, uint8_t N, uint8_t X, uint8_t Y, uint8_t Z) const noexcept
{
    const ModKey target = ModKey::createNXYZ(id, getId(), N, X, Y, Z);
    const Connection *conn = const_cast<Region*>(this)->getConnectionFromCC(cc, target);
    if (!conn)
        return {};
    return conn->sourceDepth;
}

absl::optional<sfz::ModKey::Parameters> sfz::Region::ccModParameters(int cc, ModId id, uint8_t N, uint8_t X, uint8_t Y, uint8_t Z) const noexcept
{
    const ModKey target = ModKey::createNXYZ(id, getId(), N, X, Y, Z);
    const Connection *conn = const_cast<Region*>(this)->getConnectionFromCC(cc, target);
    if (!conn)
        return {};
    return conn->source.parameters();
}
