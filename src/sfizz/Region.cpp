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

sfz::Region::Region(int regionNumber, const MidiState& midiState, absl::string_view defaultPath)
: id{regionNumber}, midiState(midiState), defaultPath(defaultPath)
{
    ccSwitched.set();

    gainToEffect.reserve(5); // sufficient room for main and fx1-4
    gainToEffect.push_back(1.0); // contribute 100% into the main bus

    // Default amplitude release
    amplitudeEG.release = Default::egRelease;
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

    #define LFO_EG_filter_EQ_target(sourceKey, targetKey, spec)                                     \
        {                                                                                           \
            const auto number = opcode.parameters.front();                                          \
            if (number == 0)                                                                        \
                return false;                                                                       \
                                                                                                    \
            const auto index = opcode.parameters.size() == 2 ? opcode.parameters.back() - 1 : 0;    \
            if (!extendIfNecessary(filters, index + 1, Default::numFilters))                        \
                return false;                                                                       \
                                                                                                    \
            const ModKey source = ModKey::createNXYZ(sourceKey, id, number - 1);                    \
            const ModKey target = ModKey::createNXYZ(targetKey, id, index);                         \
            getOrCreateConnection(source, target).sourceDepth = opcode.read(spec);                  \
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
        sampleQuality = opcode.read(Default::sampleQuality);
        break;
    case hash("direction"):
        *sampleId = sampleId->reversed(opcode.value == "reverse");
        break;
    case hash("delay"):
        delay = opcode.read(Default::delay);
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
        sampleCount = opcode.read(Default::sampleCount);
        break;
    case hash("loop_mode"): // also loopmode
        loopMode = opcode.readOptional(Default::loopMode);
        break;
    case hash("loop_end"): // also loopend
        loopRange.setEnd(opcode.read(Default::loopEnd));
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
        triggerOnNote = (opcode.value != "-1");
        keyRange.setEnd(opcode.read(Default::hiKey));
        break;
    case hash("key"):
        triggerOnNote = (opcode.value != "-1");
        {
            auto value = opcode.read(Default::key);
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
    case hash("sustain_lo"):
        sustainThreshold = opcode.read(Default::sustainThreshold);
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

    // Modulation: LFO
    case hash("lfo&_freq"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].freq = opcode.read(Default::lfoFreq);
        }
        break;
    case_any_ccN("lfo&_freq"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            processGenericCc(opcode, Default::lfoFreqMod, ModKey::createNXYZ(ModId::LFOFrequency, id, lfoNumber - 1));
        }
        break;
    case hash("lfo&_beats"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].beats = opcode.read(Default::lfoBeats);
        }
        break;
    case_any_ccN("lfo&_beats"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            processGenericCc(opcode, Default::lfoBeatsMod, ModKey::createNXYZ(ModId::LFOBeats, id, lfoNumber - 1));
        }
        break;
    case hash("lfo&_phase"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].phase0 = opcode.read(Default::lfoPhase);
        }
        break;
    case hash("lfo&_delay"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].delay = opcode.read(Default::lfoDelay);
        }
        break;
    case hash("lfo&_fade"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].fade = opcode.read(Default::lfoFade);
        }
        break;
    case hash("lfo&_count"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            lfos[lfoNumber - 1].count = opcode.read(Default::lfoCount);
        }
        break;
    case hash("lfo&_steps"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            if (!extendIfNecessary(lfos, lfoNumber, Default::numLFOs))
                return false;
            if (!lfos[lfoNumber - 1].seq)
                lfos[lfoNumber - 1].seq = LFODescription::StepSequence();
            lfos[lfoNumber - 1].seq->steps.resize(opcode.read(Default::lfoSteps));
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
            if (!lfos[lfoNumber - 1].seq)
                lfos[lfoNumber - 1].seq = LFODescription::StepSequence();
            if (!extendIfNecessary(lfos[lfoNumber - 1].seq->steps, stepNumber, Default::numLFOSteps))
                return false;
            lfos[lfoNumber - 1].seq->steps[stepNumber - 1] = opcode.read(Default::lfoStepX);
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
            if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                    return false;
            lfos[lfoNumber - 1].sub[subNumber - 1].wave = opcode.read(Default::lfoWave);
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
            if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                return false;
            lfos[lfoNumber - 1].sub[subNumber - 1].offset = opcode.read(Default::lfoOffset);
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
            if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                return false;
            lfos[lfoNumber - 1].sub[subNumber - 1].ratio = opcode.read(Default::lfoRatio);
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
            if (!extendIfNecessary(lfos[lfoNumber - 1].sub, subNumber, Default::numLFOSubs))
                return false;
            lfos[lfoNumber - 1].sub[subNumber - 1].scale = opcode.read(Default::lfoScale);
        }
        break;

    // Modulation: LFO (targets)
    case hash("lfo&_amplitude"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::amplitudeMod);
        }
        break;
    case hash("lfo&_pan"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::panMod);
        }
        break;
    case hash("lfo&_width"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Width, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::widthMod);
        }
        break;
    case hash("lfo&_position"): // sfizz extension
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Position, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::positionMod);
        }
        break;
    case hash("lfo&_pitch"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::pitchMod);
        }
        break;
    case hash("lfo&_volume"):
        {
            const auto lfoNumber = opcode.parameters.front();
            if (lfoNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::LFO, id, lfoNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Volume, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::volumeMod);
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

    // Modulation: Flex EG (targets)
    case hash("eg&_amplitude"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Amplitude, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::amplitudeMod);
        }
        break;
    case hash("eg&_pan"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Pan, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::panMod);
        }
        break;
    case hash("eg&_width"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Width, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::widthMod);
        }
        break;
    case hash("eg&_position"): // sfizz extension
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Position, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::positionMod);
        }
        break;
    case hash("eg&_pitch"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
            const ModKey target = ModKey::createNXYZ(ModId::Pitch, id);
            getOrCreateConnection(source, target).sourceDepth =
                opcode.read(Default::pitchMod);
        }
        break;
    case hash("eg&_volume"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            const ModKey source = ModKey::createNXYZ(ModId::Envelope, id, egNumber - 1);
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
        const auto egNumber = opcode.parameters.front();
        if (egNumber == 0)
            return false;
        if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
            return false;
        auto ampeg = opcode.read(Default::flexEGAmpeg);
        FlexEGDescription& desc = flexEGs[egNumber - 1];
        if (desc.ampeg != ampeg) {
            desc.ampeg = ampeg;
            flexAmpEG = absl::nullopt;
            for (size_t i = 0, n = flexEGs.size(); i < n && !flexAmpEG; ++i) {
                if (flexEGs[i].ampeg)
                    flexAmpEG = static_cast<uint8_t>(i);
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
    case hash("ampeg_veltoattack"): // also ampeg_vel2attack
    case hash("ampeg_veltodecay"): // also ampeg_vel2decay
    case hash("ampeg_veltodelay"): // also ampeg_vel2delay
    case hash("ampeg_veltohold"): // also ampeg_vel2hold
    case hash("ampeg_veltorelease"): // also ampeg_vel2release
    case hash("ampeg_veltosustain"): // also ampeg_vel2sustain
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
    case hash("pitcheg_veltoattack"): // also pitcheg_vel2attack
    case hash("pitcheg_veltodecay"): // also pitcheg_vel2decay
    case hash("pitcheg_veltodelay"): // also pitcheg_vel2delay
    case hash("pitcheg_veltohold"): // also pitcheg_vel2hold
    case hash("pitcheg_veltorelease"): // also pitcheg_vel2release
    case hash("pitcheg_veltosustain"): // also pitcheg_vel2sustain
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
    case hash("fileg_veltoattack"): // also fileg_vel2attack
    case hash("fileg_veltodecay"): // also fileg_vel2decay
    case hash("fileg_veltodelay"): // also fileg_vel2delay
    case hash("fileg_veltohold"): // also fileg_vel2hold
    case hash("fileg_veltorelease"): // also fileg_vel2release
    case hash("fileg_veltosustain"): // also fileg_vel2sustain
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

    // Flex envelopes
    case hash("eg&_dynamic"):
        {
            const auto egNumber = opcode.parameters.front();
            if (egNumber == 0)
                return false;
            if (!extendIfNecessary(flexEGs, egNumber, Default::numFlexEGs))
                return false;
            auto& eg = flexEGs[egNumber - 1];
            eg.dynamic = opcode.read(Default::flexEGDynamic);
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
            eg.sustain = opcode.read(Default::flexEGSustain);
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
            eg.points[pointNumber].time = opcode.read(Default::flexEGPointTime);
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
            eg.points[pointNumber].level = opcode.read(Default::flexEGPointLevel);
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
            eg.points[pointNumber].setShape(opcode.read(Default::flexEGPointShape));
        }
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
    const bool randOk = randRange.contains(randValue) || (randValue == 1.0f && randRange.getEnd() == 1.0f);
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
    const bool randOk = randRange.contains(randValue);

    if (!(velOk && keyOk && randOk))
        return false;

    // Release logic

    if (trigger == Trigger::release_key)
        return true;

    if (trigger == Trigger::release) {
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
    pitchVariationInCents += pitch; // sample tuning
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
