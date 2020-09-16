// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Synth.h"
#include "Config.h"
#include "Debug.h"
#include "Macros.h"
#include "MidiState.h"
#include "TriggerEvent.h"
#include "ModifierHelpers.h"
#include "ScopedFTZ.h"
#include "StringViewHelpers.h"
#include "modulations/ModMatrix.h"
#include "modulations/ModKey.h"
#include "modulations/ModId.h"
#include "modulations/sources/Controller.h"
#include "modulations/sources/LFO.h"
#include "pugixml.hpp"
#include "absl/algorithm/container.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_replace.h"
#include "SisterVoiceRing.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>

sfz::Synth::Synth()
    : Synth(config::numVoices)
{
}

sfz::Synth::Synth(int numVoices)
{
    initializeSIMDDispatchers();

    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };
    parser.setListener(this);
    effectFactory.registerStandardEffectTypes();
    effectBuses.reserve(5); // sufficient room for main and fx1-4
    resetVoices(numVoices);

    // modulation sources
    genController.reset(new ControllerSource(resources));
    genLFO.reset(new LFOSource(*this));
}

sfz::Synth::~Synth()
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    for (auto& voice : voices)
        voice->reset();

    resources.filePool.emptyFileLoadingQueues();
}

void sfz::Synth::onVoiceStateChanged(NumericId<Voice> id, Voice::State state)
{
    (void)id;
    (void)state;
    if (state == Voice::State::idle) {
        auto voice = getVoiceById(id);
        RegionSet::removeVoiceFromHierarchy(voice->getRegion(), voice);
        polyphonyGroups[voice->getRegion()->group].removeVoice(voice);
    }

}

void sfz::Synth::onParseFullBlock(const std::string& header, const std::vector<Opcode>& members)
{
    const auto newRegionSet = [&](RegionSet* parentSet) {
        ASSERT(parentSet != nullptr);
        sets.emplace_back(new RegionSet);
        auto newSet = sets.back().get();
        parentSet->addSubset(newSet);
        newSet->setParent(parentSet);
        currentSet = newSet;
    };

    switch (hash(header)) {
    case hash("global"):
        globalOpcodes = members;
        currentSet = sets.front().get();
        lastHeader = OpcodeScope::kOpcodeScopeGlobal;
        groupOpcodes.clear();
        masterOpcodes.clear();
        handleGlobalOpcodes(members);
        break;
    case hash("control"):
        defaultPath = ""; // Always reset on a new control header
        handleControlOpcodes(members);
        break;
    case hash("master"):
        masterOpcodes = members;
        newRegionSet(sets.front().get());
        groupOpcodes.clear();
        lastHeader = OpcodeScope::kOpcodeScopeMaster;
        handleMasterOpcodes(members);
        numMasters++;
        break;
    case hash("group"):
        groupOpcodes = members;
        if (lastHeader == OpcodeScope::kOpcodeScopeGroup)
            newRegionSet(currentSet->getParent());
        else
            newRegionSet(currentSet);
        lastHeader = OpcodeScope::kOpcodeScopeGroup;
        handleGroupOpcodes(members, masterOpcodes);
        numGroups++;
        break;
    case hash("region"):
        buildRegion(members);
        break;
    case hash("curve"):
        resources.curves.addCurveFromHeader(members);
        break;
    case hash("effect"):
        handleEffectOpcodes(members);
        break;
    default:
        std::cerr << "Unknown header: " << header << '\n';
    }
}

void sfz::Synth::onParseError(const SourceRange& range, const std::string& message)
{
    const auto relativePath = range.start.filePath->lexically_relative(parser.originalDirectory());
    std::cerr << "Parse error in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
}

void sfz::Synth::onParseWarning(const SourceRange& range, const std::string& message)
{
    const auto relativePath = range.start.filePath->lexically_relative(parser.originalDirectory());
    std::cerr << "Parse warning in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
}

void sfz::Synth::buildRegion(const std::vector<Opcode>& regionOpcodes)
{
    ASSERT(currentSet != nullptr);

    int regionNumber = static_cast<int>(regions.size());
    auto lastRegion = absl::make_unique<Region>(regionNumber, resources.midiState, defaultPath);

    // Create default connections
    constexpr unsigned defaultSmoothness = 10;
    lastRegion->getOrCreateConnection(
        ModKey::createCC(7, 4, defaultSmoothness, 100, 0),
        ModKey::createNXYZ(ModId::Amplitude, lastRegion->id));
    lastRegion->getOrCreateConnection(
        ModKey::createCC(10, 1, defaultSmoothness, 100, 0),
        ModKey::createNXYZ(ModId::Pan, lastRegion->id));

    //
    auto parseOpcodes = [&](const std::vector<Opcode>& opcodes) {
        for (auto& opcode : opcodes) {
            const auto unknown = absl::c_find_if(unknownOpcodes, [&](absl::string_view sv) { return sv.compare(opcode.opcode) == 0; });
            if (unknown != unknownOpcodes.end()) {
                continue;
            }

            if (!lastRegion->parseOpcode(opcode))
                unknownOpcodes.emplace_back(opcode.opcode);
        }
    };

    parseOpcodes(globalOpcodes);
    parseOpcodes(masterOpcodes);
    parseOpcodes(groupOpcodes);
    parseOpcodes(regionOpcodes);

    if (octaveOffset != 0 || noteOffset != 0)
        lastRegion->offsetAllKeys(octaveOffset * 12 + noteOffset);

    // There was a combination of group= and polyphony= on a region, so set the group polyphony
    if (lastRegion->group != Default::group && lastRegion->polyphony != config::maxVoices)
        setGroupPolyphony(lastRegion->group, lastRegion->polyphony);

    lastRegion->parent = currentSet;
    currentSet->addRegion(lastRegion.get());

    // Adapt the size of the delayed releases to avoid allocating later on
    lastRegion->delayedReleases.reserve(lastRegion->keyRange.length());

    regions.push_back(std::move(lastRegion));
}

void sfz::Synth::clear()
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    for (auto& voice : voices)
        voice->reset();
    for (auto& list : noteActivationLists)
        list.clear();
    for (auto& list : ccActivationLists)
        list.clear();

    lastHeader = OpcodeScope::kOpcodeScopeGlobal;
    sets.clear();
    sets.emplace_back(new RegionSet);
    currentSet = sets.front().get();
    regions.clear();
    effectBuses.clear();
    effectBuses.emplace_back(new EffectBus);
    effectBuses[0]->setGainToMain(1.0);
    effectBuses[0]->setSamplesPerBlock(samplesPerBlock);
    effectBuses[0]->setSampleRate(sampleRate);
    effectBuses[0]->clearInputs(samplesPerBlock);
    resources.clear();
    numGroups = 0;
    numMasters = 0;
    defaultSwitch = absl::nullopt;
    defaultPath = "";
    resources.midiState.reset();
    ccLabels.clear();
    keyLabels.clear();
    keyswitchLabels.clear();
    globalOpcodes.clear();
    masterOpcodes.clear();
    groupOpcodes.clear();
    unknownOpcodes.clear();
    polyphonyGroups.clear();
    polyphonyGroups.emplace_back();
    polyphonyGroups.back().setPolyphonyLimit(config::maxVoices);
    modificationTime = fs::file_time_type::min();

    // set default controllers
    cc(0, 7, 100);     // volume
    hdcc(0, 10, 0.5f); // pan

    // set default controller labels
    ccLabels.emplace_back(7, "Volume");
    ccLabels.emplace_back(10, "Pan");
}

void sfz::Synth::handleMasterOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeMaster);

        switch (member.lettersOnlyHash) {
        case hash("polyphony"):
            ASSERT(currentSet != nullptr);
            if (auto value = readOpcode(member.value, Default::polyphonyRange))
                currentSet->setPolyphonyLimit(*value);
            break;
        case hash("sw_default"):
            setValueFromOpcode(member, defaultSwitch, Default::keyRange);
            break;
        }
    }
}

void sfz::Synth::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeGlobal);

        switch (member.lettersOnlyHash) {
        case hash("polyphony"):
            ASSERT(currentSet != nullptr);
            if (auto value = readOpcode(member.value, Default::polyphonyRange))
                currentSet->setPolyphonyLimit(*value);
            break;
        case hash("sw_default"):
            setValueFromOpcode(member, defaultSwitch, Default::keyRange);
            break;
        case hash("volume"):
            // FIXME : Probably best not to mess with this and let the host control the volume
            // setValueFromOpcode(member, volume, Default::volumeRange);
            break;
        }
    }
}

void sfz::Synth::handleGroupOpcodes(const std::vector<Opcode>& members, const std::vector<Opcode>& masterMembers)
{
    absl::optional<unsigned> groupIdx;
    absl::optional<unsigned> maxPolyphony;

    const auto parseOpcode = [&](const Opcode& rawMember) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeGroup);

        switch (member.lettersOnlyHash) {
        case hash("group"):
            setValueFromOpcode(member, groupIdx, Default::groupRange);
            break;
        case hash("polyphony"):
            setValueFromOpcode(member, maxPolyphony, Default::polyphonyRange);
            break;
        case hash("sw_default"):
            setValueFromOpcode(member, defaultSwitch, Default::keyRange);
            break;
        }
    };

    for (auto& member : masterMembers)
        parseOpcode(member);

    for (auto& member : members)
        parseOpcode(member);

    if (groupIdx && maxPolyphony) {
        setGroupPolyphony(*groupIdx, *maxPolyphony);
    } else if (maxPolyphony) {
        ASSERT(currentSet != nullptr);
        currentSet->setPolyphonyLimit(*maxPolyphony);
    } else if (groupIdx && *groupIdx > polyphonyGroups.size()) {
        setGroupPolyphony(*groupIdx, config::maxVoices);
    }
}

void sfz::Synth::handleControlOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeControl);

        switch (member.lettersOnlyHash) {
        case hash("set_cc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back())) {
                const auto ccValue = readOpcode(member.value, Default::midi7Range);
                if (ccValue)
                    resources.midiState.ccEvent(0, member.parameters.back(), normalizeCC(*ccValue));
            }
            break;
        case hash("set_hdcc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back())) {
                const auto ccValue = readOpcode(member.value, Default::normalizedRange);
                if (ccValue)
                    resources.midiState.ccEvent(0, member.parameters.back(), *ccValue);
            }
            break;
        case hash("label_cc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back()))
                ccLabels.emplace_back(member.parameters.back(), std::string(member.value));
            break;
        case hash("label_key&"):
            if (member.parameters.back() <= Default::keyRange.getEnd()) {
                const auto noteNumber = static_cast<uint8_t>(member.parameters.back());
                keyLabels.emplace_back(noteNumber, std::string(member.value));
            }
            break;
        case hash("default_path"):
            defaultPath = absl::StrReplaceAll(trim(member.value), { { "\\", "/" } });
            DBG("Changing default sample path to " << defaultPath);
            break;
        case hash("note_offset"):
            setValueFromOpcode(member, noteOffset, Default::noteOffsetRange);
            break;
        case hash("octave_offset"):
            setValueFromOpcode(member, octaveOffset, Default::octaveOffsetRange);
            break;
        default:
            // Unsupported control opcode
            DBG("Unsupported control opcode: " << member.opcode);
        }
    }
}

void sfz::Synth::handleEffectOpcodes(const std::vector<Opcode>& rawMembers)
{
    absl::string_view busName = "main";

    auto getOrCreateBus = [this](unsigned index) -> EffectBus& {
        if (index + 1 > effectBuses.size())
            effectBuses.resize(index + 1);
        EffectBusPtr& bus = effectBuses[index];
        if (!bus) {
            bus.reset(new EffectBus);
            bus->setSampleRate(sampleRate);
            bus->setSamplesPerBlock(samplesPerBlock);
            bus->clearInputs(samplesPerBlock);
        }
        return *bus;
    };

    std::vector<Opcode> members;
    members.reserve(rawMembers.size());
    for (const Opcode& opcode : rawMembers)
        members.push_back(opcode.cleanUp(kOpcodeScopeEffect));

    for (const Opcode& opcode : members) {
        switch (opcode.lettersOnlyHash) {
        case hash("bus"):
            busName = opcode.value;
            break;

            // note(jpc): gain opcodes are linear volumes in % units

        case hash("directtomain"):
            if (auto valueOpt = readOpcode<float>(opcode.value, { 0, 100 }))
                getOrCreateBus(0).setGainToMain(*valueOpt / 100);
            break;

        case hash("fx&tomain"): // fx&tomain
            if (opcode.parameters.front() < 1 || opcode.parameters.front() > config::maxEffectBuses)
                break;
            if (auto valueOpt = readOpcode<float>(opcode.value, { 0, 100 }))
                getOrCreateBus(opcode.parameters.front()).setGainToMain(*valueOpt / 100);
            break;

        case hash("fx&tomix"): // fx&tomix
            if (opcode.parameters.front() < 1 || opcode.parameters.front() > config::maxEffectBuses)
                break;
            if (auto valueOpt = readOpcode<float>(opcode.value, { 0, 100 }))
                getOrCreateBus(opcode.parameters.front()).setGainToMix(*valueOpt / 100);
            break;
        }
    }

    unsigned busIndex;
    if (busName.empty() || busName == "main")
        busIndex = 0;
    else if (busName.size() > 2 && busName.substr(0, 2) == "fx" && absl::SimpleAtoi(busName.substr(2), &busIndex) && busIndex >= 1 && busIndex <= config::maxEffectBuses) {
        // an effect bus fxN, with N usually in [1,4]
    } else {
        DBG("Unsupported effect bus: " << busName);
        return;
    }

    // create the effect and add it
    EffectBus& bus = getOrCreateBus(busIndex);
    auto fx = effectFactory.makeEffect(members);
    fx->setSampleRate(sampleRate);
    fx->setSamplesPerBlock(samplesPerBlock);
    bus.addEffect(std::move(fx));
}

bool sfz::Synth::loadSfzFile(const fs::path& file)
{
    clear();

    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    std::error_code ec;
    fs::path realFile = fs::canonical(file, ec);

    parser.parseFile(ec ? file : realFile);
    if (parser.getErrorCount() > 0)
        return false;

    if (regions.empty())
        return false;

    finalizeSfzLoad();

    return true;
}

bool sfz::Synth::loadSfzString(const fs::path& path, absl::string_view text)
{
    clear();

    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };
    parser.parseString(path, text);
    if (parser.getErrorCount() > 0)
        return false;

    if (regions.empty())
        return false;

    finalizeSfzLoad();

    return true;
}

void sfz::Synth::finalizeSfzLoad()
{
    resources.filePool.setRootDirectory(parser.originalDirectory());

    size_t currentRegionIndex = 0;
    size_t currentRegionCount = regions.size();

    auto removeCurrentRegion = [this, &currentRegionIndex, &currentRegionCount]() {
        DBG("Removing the region with sample " << regions[currentRegionIndex]->sampleId);
        regions.erase(regions.begin() + currentRegionIndex);
        --currentRegionCount;
    };

    size_t maxFilters { 0 };
    size_t maxEQs { 0 };
    size_t maxLFOs { 0 };

    while (currentRegionIndex < currentRegionCount) {
        auto region = regions[currentRegionIndex].get();

        if (!region->oscillator && !region->isGenerator()) {
            if (!resources.filePool.checkSampleId(region->sampleId)) {
                removeCurrentRegion();
                continue;
            }

            const auto fileInformation = resources.filePool.getFileInformation(region->sampleId);
            if (!fileInformation) {
                removeCurrentRegion();
                continue;
            }

            region->sampleEnd = std::min(region->sampleEnd, fileInformation->end);

            if (fileInformation->hasLoop) {
                if (region->loopRange.getStart() == Default::loopRange.getStart())
                    region->loopRange.setStart(fileInformation->loopBegin);

                if (region->loopRange.getEnd() == Default::loopRange.getEnd())
                    region->loopRange.setEnd(fileInformation->loopEnd);

                if (!region->loopMode)
                    region->loopMode = SfzLoopMode::loop_continuous;
            }

            if (region->isRelease() && !region->loopMode)
                region->loopMode = SfzLoopMode::one_shot;

            if (region->loopRange.getEnd() == Default::loopRange.getEnd())
                region->loopRange.setEnd(region->sampleEnd);

            if (fileInformation->numChannels == 2)
                region->hasStereoSample = true;

            // TODO: adjust with LFO targets
            const auto maxOffset = [region]() {
                uint64_t sumOffsetCC = region->offset + region->offsetRandom;
                for (const auto& offsets : region->offsetCC)
                    sumOffsetCC += offsets.data;
                return Default::offsetCCRange.clamp(sumOffsetCC);
            }();

            if (!resources.filePool.preloadFile(region->sampleId, maxOffset))
                removeCurrentRegion();
        } else if (region->oscillator && !region->isGenerator()) {
            if (!resources.filePool.checkSampleId(region->sampleId)) {
                removeCurrentRegion();
                continue;
            }

            if (!resources.wavePool.createFileWave(resources.filePool, std::string(region->sampleId.filename()))) {
                removeCurrentRegion();
                continue;
            }
        }

        if (region->keyswitchLabel && region->keyswitch)
            keyswitchLabels.push_back({ *region->keyswitch, *region->keyswitchLabel });

        // Some regions had group number but no "group-level" opcodes handled the polyphony
        while (polyphonyGroups.size() <= region->group) {
            polyphonyGroups.emplace_back();
            polyphonyGroups.back().setPolyphonyLimit(config::maxVoices);
        }

        for (auto note = 0; note < 128; note++) {
            if (region->keyRange.containsWithEnd(note) || (region->hasKeyswitches() && region->keyswitchRange.containsWithEnd(note)))
                noteActivationLists[note].push_back(region);
        }

        for (int cc = 0; cc < config::numCCs; cc++) {
            if (region->ccTriggers.contains(cc)
                || region->ccConditions.contains(cc)
                || (cc == region->sustainCC && region->trigger == SfzTrigger::release))
                ccActivationLists[cc].push_back(region);
        }

        // Defaults
        for (int cc = 0; cc < config::numCCs; cc++) {
            region->registerCC(cc, resources.midiState.getCCValue(cc));
        }

        if (defaultSwitch) {
            region->registerNoteOn(*defaultSwitch, 1.0f, 1.0f);
            region->registerNoteOff(*defaultSwitch, 0.0f, 1.0f);
        }

        // Set the default frequencies on equalizers if needed
        if (region->equalizers.size() > 0
            && region->equalizers[0].frequency == Default::eqFrequencyUnset) {
            region->equalizers[0].frequency = Default::eqFrequency1;
            if (region->equalizers.size() > 1
                && region->equalizers[1].frequency == Default::eqFrequencyUnset) {
                region->equalizers[1].frequency = Default::eqFrequency2;
                if (region->equalizers.size() > 2
                    && region->equalizers[2].frequency == Default::eqFrequencyUnset) {
                    region->equalizers[2].frequency = Default::eqFrequency3;
                }
            }
        }

        if (!region->velocityPoints.empty())
            region->velCurve = Curve::buildFromVelcurvePoints(
                region->velocityPoints, Curve::Interpolator::Linear);

        region->registerPitchWheel(0);
        region->registerAftertouch(0);
        region->registerTempo(2.0f);
        maxFilters = max(maxFilters, region->filters.size());
        maxEQs = max(maxEQs, region->equalizers.size());
        maxLFOs = max(maxLFOs, region->lfos.size());

        ++currentRegionIndex;
    }
    DBG("Removing " << (regions.size() - currentRegionCount) << " out of " << regions.size() << " regions");
    regions.resize(currentRegionCount);
    modificationTime = checkModificationTime();

    settingsPerVoice.maxFilters = maxFilters;
    settingsPerVoice.maxEQs = maxEQs;
    settingsPerVoice.maxLFOs = maxLFOs;

    applySettingsPerVoice();

    setupModMatrix();
}

bool sfz::Synth::loadScalaFile(const fs::path& path)
{
    return resources.tuning.loadScalaFile(path);
}

bool sfz::Synth::loadScalaString(const std::string& text)
{
    return resources.tuning.loadScalaString(text);
}

void sfz::Synth::setScalaRootKey(int rootKey)
{
    resources.tuning.setScalaRootKey(rootKey);
}

int sfz::Synth::getScalaRootKey() const
{
    return resources.tuning.getScalaRootKey();
}

void sfz::Synth::setTuningFrequency(float frequency)
{
    resources.tuning.setTuningFrequency(frequency);
}

float sfz::Synth::getTuningFrequency() const
{
    return resources.tuning.getTuningFrequency();
}

void sfz::Synth::loadStretchTuningByRatio(float ratio)
{
    SFIZZ_CHECK(ratio >= 0.0f && ratio <= 1.0f);
    ratio = clamp(ratio, 0.0f, 1.0f);

    if (ratio > 0.0f)
        resources.stretch = StretchTuning::createRailsbackFromRatio(ratio);
    else
        resources.stretch.reset();
}

sfz::Voice* sfz::Synth::findFreeVoice() noexcept
{
    auto freeVoice = absl::c_find_if(voices, [](const std::unique_ptr<Voice>& voice) {
        return voice->isFree();
    });

    if (freeVoice != voices.end())
        return freeVoice->get();

    // Engine polyphony reached
    Voice* stolenVoice = stealer.steal(absl::MakeSpan(voiceViewArray));
    if (stolenVoice == nullptr)
        return {};

    // Never kill age 0 voices
    if (stolenVoice->getAge() == 0)
        return {};


    auto tempSpan = resources.bufferPool.getStereoBuffer(samplesPerBlock);
    SisterVoiceRing::applyToRing(stolenVoice, [&] (Voice* v) {
        renderVoiceToOutputs(*v, *tempSpan);
        v->reset();
    });

    return stolenVoice;
}

int sfz::Synth::getNumActiveVoices(bool recompute) const noexcept
{
    if (!recompute)
        return activeVoices;

    int active { 0 };
    for (auto& voice: voices) {
        if (!voice->isFree())
            active++;
    }

    return active;
}

void sfz::Synth::garbageCollect() noexcept
{
}

void sfz::Synth::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    ASSERT(samplesPerBlock < config::maxBlockSize);

    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    this->samplesPerBlock = samplesPerBlock;
    for (auto& voice : voices)
        voice->setSamplesPerBlock(samplesPerBlock);

    resources.setSamplesPerBlock(samplesPerBlock);

    for (auto& bus : effectBuses) {
        if (bus)
            bus->setSamplesPerBlock(samplesPerBlock);
    }
}

void sfz::Synth::setSampleRate(float sampleRate) noexcept
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    this->sampleRate = sampleRate;
    for (auto& voice : voices)
        voice->setSampleRate(sampleRate);

    resources.setSampleRate(sampleRate);

    for (auto& bus : effectBuses) {
        if (bus)
            bus->setSampleRate(sampleRate);
    }
}

void sfz::Synth::renderVoiceToOutputs(Voice& voice, AudioSpan<float>& tempSpan) noexcept
{
    const Region* region = voice.getRegion();
    ASSERT(region != nullptr);

    voice.renderBlock(tempSpan);
    for (size_t i = 0, n = effectBuses.size(); i < n; ++i) {
        if (auto& bus = effectBuses[i]) {
            float addGain = region->getGainToEffectBus(i);
            bus->addToInputs(tempSpan, addGain, tempSpan.getNumFrames());
        }
    }
}

void sfz::Synth::renderBlock(AudioSpan<float> buffer) noexcept
{
    ScopedFTZ ftz;
    CallbackBreakdown callbackBreakdown;

    { // Silence buffer
        ScopedTiming logger { callbackBreakdown.renderMethod };
        buffer.fill(0.0f);
    }

    if (resources.synthConfig.freeWheeling)
        resources.filePool.waitForBackgroundLoading();

    const std::unique_lock<SpinMutex> lock { callbackGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    size_t numFrames = buffer.getNumFrames();
    auto tempSpan = resources.bufferPool.getStereoBuffer(numFrames);
    auto tempMixSpan = resources.bufferPool.getStereoBuffer(numFrames);
    auto rampSpan = resources.bufferPool.getBuffer(numFrames);
    if (!tempSpan || !tempMixSpan || !rampSpan) {
        DBG("[sfizz] Could not get a temporary buffer; exiting callback... ");
        return;
    }

    ModMatrix& mm = resources.modMatrix;
    mm.beginCycle(numFrames);

    activeVoices = 0;
    { // Main render block
        ScopedTiming logger { callbackBreakdown.renderMethod, ScopedTiming::Operation::addToDuration };
        tempSpan->fill(0.0f);
        tempMixSpan->fill(0.0f);
        resources.filePool.cleanupPromises();

        // Ramp out whatever is in the buffer at this point; should only be killed voice data
        linearRamp<float>(*rampSpan, 1.0f, -1.0f / static_cast<float>(numFrames));
        for (size_t i = 0, n = effectBuses.size(); i < n; ++i) {
            if (auto& bus = effectBuses[i]) {
                bus->applyGain(rampSpan->data(), numFrames);
            }
        }

        for (auto& voice : voices) {
            if (voice->isFree())
                continue;

            mm.beginVoice(voice->getId(), voice->getRegion()->getId());

            activeVoices++;
            renderVoiceToOutputs(*voice, *tempSpan);
            callbackBreakdown.data += voice->getLastDataDuration();
            callbackBreakdown.amplitude += voice->getLastAmplitudeDuration();
            callbackBreakdown.filters += voice->getLastFilterDuration();
            callbackBreakdown.panning += voice->getLastPanningDuration();

            mm.endVoice();

            if (voice->toBeCleanedUp())
                voice->reset();
        }
    }

    { // Apply effect buses
        // -- note(jpc) there is always a "main" bus which is initially empty.
        //    without any <effect>, the signal is just going to flow through it.
        ScopedTiming logger { callbackBreakdown.effects, ScopedTiming::Operation::addToDuration };

        for (auto& bus : effectBuses) {
            if (bus) {
                bus->process(numFrames);
                bus->mixOutputsTo(buffer, *tempMixSpan, numFrames);
            }
        }
    }

    // Add the Mix output (fxNtomix opcodes)
    // -- note(jpc) the purpose of the Mix output is not known.
    //    perhaps it's designed as extension point for custom processing?
    //    as default behavior, it adds itself to the Main signal.
    buffer.add(*tempMixSpan);

    // Apply the master volume
    buffer.applyGain(db2mag(volume));

    // Perform any remaining modulators
    mm.endCycle();

    { // Clear events and advance midi time
        ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
        resources.midiState.advanceTime(buffer.getNumFrames());
    }

    callbackBreakdown.dispatch = dispatchDuration;
    resources.logger.logCallbackTime(callbackBreakdown, activeVoices, numFrames);

    // Reset the dispatch counter
    dispatchDuration = Duration(0);

    { // Clear for the next run
        ScopedTiming logger { callbackBreakdown.effects };
        for (auto& bus : effectBuses) {
            if (bus)
                bus->clearInputs(numFrames);
        }
    }

    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
}

void sfz::Synth::noteOn(int delay, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    const auto normalizedVelocity = normalizeVelocity(velocity);
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
    resources.midiState.noteOnEvent(delay, noteNumber, normalizedVelocity);

    const std::unique_lock<SpinMutex> lock { callbackGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    noteOnDispatch(delay, noteNumber, normalizedVelocity);
}

void sfz::Synth::noteOff(int delay, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    UNUSED(velocity);
    const auto normalizedVelocity = normalizeVelocity(velocity);
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
    resources.midiState.noteOffEvent(delay, noteNumber, normalizedVelocity);

    const std::unique_lock<SpinMutex> lock { callbackGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    // FIXME: Some keyboards (e.g. Casio PX5S) can send a real note-off velocity. In this case, do we have a
    // way in sfz to specify that a release trigger should NOT use the note-on velocity?
    // auto replacedVelocity = (velocity == 0 ? sfz::getNoteVelocity(noteNumber) : velocity);
    const auto replacedVelocity = resources.midiState.getNoteVelocity(noteNumber);

    for (auto& voice : voices)
        voice->registerNoteOff(delay, noteNumber, replacedVelocity);

    noteOffDispatch(delay, noteNumber, replacedVelocity);
}

void sfz::Synth::startVoice(Region* region, int delay, const TriggerEvent& triggerEvent, SisterVoiceRingBuilder& ring) noexcept
{
    checkNotePolyphony(region, delay, triggerEvent);
    checkRegionPolyphony(region, delay);
    checkGroupPolyphony(region, delay);
    checkSetPolyphony(region, delay);

    Voice* selectedVoice = findFreeVoice();
    if (selectedVoice == nullptr)
        return;

    ASSERT(selectedVoice->isFree());
    selectedVoice->startVoice(region, delay, triggerEvent);
    ring.addVoiceToRing(selectedVoice);
    RegionSet::registerVoiceInHierarchy(region, selectedVoice);
    polyphonyGroups[region->group].registerVoice(selectedVoice);
}

bool sfz::Synth::playingAttackVoice(const Region* releaseRegion) noexcept
{
    const auto compatibleVoice = [releaseRegion](const Voice* v) -> bool {
        const sfz::TriggerEvent& event = v->getTriggerEvent();
        return (
            !v->isFree()
            && event.type == sfz::TriggerEventType::NoteOn
            && releaseRegion->keyRange.containsWithEnd(event.number)
            && releaseRegion->velocityRange.containsWithEnd(event.value)
        );
    };

    if (absl::c_find_if(voiceViewArray, compatibleVoice) == voiceViewArray.end())
        return false;
    else
        return true;
}

void sfz::Synth::noteOffDispatch(int delay, int noteNumber, float velocity) noexcept
{
    const auto randValue = randNoteDistribution(Random::randomGenerator);
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::NoteOff, noteNumber, velocity };

    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOff(noteNumber, velocity, randValue)) {
            if (region->trigger == SfzTrigger::release && !region->rtDead && !playingAttackVoice(region))
                continue;

            startVoice(region, delay, triggerEvent, ring);
        }
    }
}

void sfz::Synth::checkRegionPolyphony(const Region* region, int delay) noexcept
{
    tempPolyphonyArray.clear();

    for (Voice* voice : voiceViewArray) {
        if (voice->getRegion() == region && !voice->releasedOrFree()) {
            tempPolyphonyArray.push_back(voice);
        }
    }

    if (tempPolyphonyArray.size() >= region->polyphony) {
        const auto voiceToSteal = stealer.steal(absl::MakeSpan(tempPolyphonyArray));
        SisterVoiceRing::offAllSisters(voiceToSteal, delay);
    }
}

void sfz::Synth::checkNotePolyphony(const Region* region, int delay, const TriggerEvent& triggerEvent) noexcept
{
    if (!region->notePolyphony)
        return;

    unsigned notePolyphonyCounter { 0 };
    Voice* selfMaskCandidate { nullptr };

    for (Voice* voice : voiceViewArray) {
        const sfz::TriggerEvent& voiceTriggerEvent = voice->getTriggerEvent();
        const bool skipVoice = (triggerEvent.type == TriggerEventType::NoteOn && voice->releasedOrFree()) || voice->isFree();
        if (!skipVoice
            && voice->getRegion()->group == region->group
            && voiceTriggerEvent.number == triggerEvent.number
            && voiceTriggerEvent.type == triggerEvent.type) {
            notePolyphonyCounter += 1;
            switch (region->selfMask) {
            case SfzSelfMask::mask:
                if (voiceTriggerEvent.value <= triggerEvent.value) {
                    if (!selfMaskCandidate || selfMaskCandidate->getTriggerEvent().value > voiceTriggerEvent.value) {
                        selfMaskCandidate = voice;
                    }
                }
                break;
            case SfzSelfMask::dontMask:
                if (!selfMaskCandidate || selfMaskCandidate->getSourcePosition() < voice->getSourcePosition())
                    selfMaskCandidate = voice;
                break;
            }
        }
    }

    if (notePolyphonyCounter >= *region->notePolyphony && selfMaskCandidate) {
        SisterVoiceRing::offAllSisters(selfMaskCandidate, delay);
    }
}

void sfz::Synth::checkGroupPolyphony(const Region* region, int delay) noexcept
{
    const auto& activeVoices = polyphonyGroups[region->group].getActiveVoices();
    tempPolyphonyArray.clear();
    for (Voice* voice : activeVoices) {
        if (!voice->releasedOrFree()) {
            tempPolyphonyArray.push_back(voice);
        }
    }

    if (tempPolyphonyArray.size() >= polyphonyGroups[region->group].getPolyphonyLimit()) {
        const auto voiceToSteal = stealer.steal(absl::MakeSpan(tempPolyphonyArray));
        SisterVoiceRing::offAllSisters(voiceToSteal, delay);
    }
}

void sfz::Synth::checkSetPolyphony(const Region* region, int delay) noexcept
{
    auto parent = region->parent;
    while (parent != nullptr) {
        const auto& activeVoices = parent->getActiveVoices();
        tempPolyphonyArray.clear();
        for (Voice* voice : activeVoices) {
            if (!voice->releasedOrFree()) {
                tempPolyphonyArray.push_back(voice);
            }
        }

        if (tempPolyphonyArray.size() >= parent->getPolyphonyLimit()) {
            const auto voiceToSteal = stealer.steal(absl::MakeSpan(tempPolyphonyArray));
            SisterVoiceRing::offAllSisters(voiceToSteal, delay);
        }

        parent = parent->getParent();
    }
}

void sfz::Synth::noteOnDispatch(int delay, int noteNumber, float velocity) noexcept
{
    const auto randValue = randNoteDistribution(Random::randomGenerator);
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::NoteOn, noteNumber, velocity };

    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOn(noteNumber, velocity, randValue)) {
            for (auto& voice : voices) {
                if (voice->checkOffGroup(region, delay, noteNumber)) {
                    const TriggerEvent& event = voice->getTriggerEvent();
                    noteOffDispatch(delay, event.number, event.value);
                }
            }

            startVoice(region, delay, triggerEvent, ring);
        }
    }
}

void sfz::Synth::startDelayedReleaseVoices(Region* region, int delay, SisterVoiceRingBuilder& ring) noexcept
{
    if (!region->rtDead && !playingAttackVoice(region)) {
        region->delayedReleases.clear();
        return;
    }

    for (auto& note: region->delayedReleases) {
        // FIXME: we really need to have some form of common method to find and start voices...
        const TriggerEvent noteOffEvent { TriggerEventType::NoteOff, note.first, note.second };
        startVoice(region, delay, noteOffEvent, ring);
    }
    region->delayedReleases.clear();
}


void sfz::Synth::cc(int delay, int ccNumber, uint8_t ccValue) noexcept
{
    const auto normalizedCC = normalizeCC(ccValue);
    hdcc(delay, ccNumber, normalizedCC);
}

void sfz::Synth::ccDispatch(int delay, int ccNumber, float value) noexcept
{
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::CC, ccNumber, value };
    for (auto& region : ccActivationLists[ccNumber]) {
        if (ccNumber == region->sustainCC)
            startDelayedReleaseVoices(region, delay, ring);

        if (region->registerCC(ccNumber, value))
            startVoice(region, delay, triggerEvent, ring);
    }
}

void sfz::Synth::hdcc(int delay, int ccNumber, float normValue) noexcept
{
    ASSERT(ccNumber < config::numCCs);
    ASSERT(ccNumber >= 0);

    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
    resources.midiState.ccEvent(delay, ccNumber, normValue);

    const std::unique_lock<SpinMutex> lock { callbackGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    if (ccNumber == config::resetCC) {
        resetAllControllers(delay);
        return;
    }

    if (ccNumber == config::allNotesOffCC || ccNumber == config::allSoundOffCC) {
        for (auto& voice : voices)
            voice->reset();
        resources.midiState.allNotesOff(delay);
        return;
    }

    for (auto& voice : voices)
        voice->registerCC(delay, ccNumber, normValue);

    ccDispatch(delay, ccNumber, normValue);
}

void sfz::Synth::pitchWheel(int delay, int pitch) noexcept
{
    ASSERT(pitch <= 8192);
    ASSERT(pitch >= -8192);
    const auto normalizedPitch = normalizeBend(float(pitch));

    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
    resources.midiState.pitchBendEvent(delay, normalizedPitch);

    for (auto& region : regions) {
        region->registerPitchWheel(normalizedPitch);
    }

    for (auto& voice : voices) {
        voice->registerPitchWheel(delay, normalizedPitch);
    }
}
void sfz::Synth::aftertouch(int /* delay */, uint8_t /* aftertouch */) noexcept
{
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
}
void sfz::Synth::tempo(int /* delay */, float /* secondsPerQuarter */) noexcept
{
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };
}
void sfz::Synth::timeSignature(int delay, int beatsPerBar, int beatUnit)
{
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)beatsPerBar;
    (void)beatUnit;
}
void sfz::Synth::timePosition(int delay, int bar, float barBeat)
{
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)bar;
    (void)barBeat;
}
void sfz::Synth::playbackState(int delay, int playbackState)
{
    ScopedTiming logger { dispatchDuration, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)playbackState;
}

int sfz::Synth::getNumRegions() const noexcept
{
    return static_cast<int>(regions.size());
}
int sfz::Synth::getNumGroups() const noexcept
{
    return numGroups;
}
int sfz::Synth::getNumMasters() const noexcept
{
    return numMasters;
}
int sfz::Synth::getNumCurves() const noexcept
{
    return static_cast<int>(resources.curves.getNumCurves());
}

std::string sfz::Synth::exportMidnam(absl::string_view model) const
{
    pugi::xml_document doc;
    absl::string_view manufacturer = config::midnamManufacturer;

    if (model.empty())
        model = config::midnamModel;

    doc.append_child(pugi::node_doctype).set_value("MIDINameDocument PUBLIC"
                                                   " \"-//MIDI Manufacturers Association//DTD MIDINameDocument 1.0//EN\""
                                                   " \"http://www.midi.org/dtds/MIDINameDocument10.dtd\"");

    pugi::xml_node root = doc.append_child("MIDINameDocument");

    root.append_child(pugi::node_comment)
        .set_value("Generated by Sfizz for the current instrument");

    root.append_child("Author");

    pugi::xml_node device = root.append_child("MasterDeviceNames");
    device.append_child("Manufacturer")
        .append_child(pugi::node_pcdata)
        .set_value(std::string(manufacturer).c_str());
    device.append_child("Model")
        .append_child(pugi::node_pcdata)
        .set_value(std::string(model).c_str());

    {
        pugi::xml_node devmode = device.append_child("CustomDeviceMode");
        devmode.append_attribute("Name").set_value("Default");

        pugi::xml_node nsas = devmode.append_child("ChannelNameSetAssignments");
        for (unsigned c = 0; c < 16; ++c) {
            pugi::xml_node nsa = nsas.append_child("ChannelNameSetAssign");
            nsa.append_attribute("Channel").set_value(std::to_string(c + 1).c_str());
            nsa.append_attribute("NameSet").set_value("Play");
        }
    }

    {
        pugi::xml_node chns = device.append_child("ChannelNameSet");
        chns.append_attribute("Name").set_value("Play");

        pugi::xml_node acs = chns.append_child("AvailableForChannels");
        for (unsigned c = 0; c < 16; ++c) {
            pugi::xml_node ac = acs.append_child("AvailableChannel");
            ac.append_attribute("Channel").set_value(std::to_string(c + 1).c_str());
            ac.append_attribute("Available").set_value("true");
        }

        chns.append_child("UsesControlNameList")
            .append_attribute("Name")
            .set_value("Controls");
        chns.append_child("UsesNoteNameList")
            .append_attribute("Name")
            .set_value("Notes");
    }

    {
        auto anonymousCCs = getUsedCCs();

        pugi::xml_node cns = device.append_child("ControlNameList");
        cns.append_attribute("Name").set_value("Controls");
        for (const auto& pair : ccLabels) {
            anonymousCCs.set(pair.first, false);
            if (pair.first < 128) {
                pugi::xml_node cn = cns.append_child("Control");
                cn.append_attribute("Type").set_value("7bit");
                cn.append_attribute("Number").set_value(std::to_string(pair.first).c_str());
                cn.append_attribute("Name").set_value(pair.second.c_str());
            }
        }

        for (unsigned i = 0, n = std::min<unsigned>(128, anonymousCCs.size()); i < n; ++i) {
            if (anonymousCCs[i]) {
                pugi::xml_node cn = cns.append_child("Control");
                cn.append_attribute("Type").set_value("7bit");
                cn.append_attribute("Number").set_value(std::to_string(i).c_str());
                cn.append_attribute("Name").set_value(("Unnamed CC " + std::to_string(i)).c_str());
            }
        }
    }

    {
        pugi::xml_node nnl = device.append_child("NoteNameList");
        nnl.append_attribute("Name").set_value("Notes");
        for (const auto& pair : keyswitchLabels) {
            pugi::xml_node nn = nnl.append_child("Note");
            nn.append_attribute("Number").set_value(std::to_string(pair.first).c_str());
            nn.append_attribute("Name").set_value(pair.second.c_str());
        }
        for (const auto& pair : keyLabels) {
            pugi::xml_node nn = nnl.append_child("Note");
            nn.append_attribute("Number").set_value(std::to_string(pair.first).c_str());
            nn.append_attribute("Name").set_value(pair.second.c_str());
        }
    }

    ///
    struct string_writer : pugi::xml_writer {
        std::string result;

        string_writer()
        {
            result.reserve(8192);
        }

        void write(const void* data, size_t size) override
        {
            result.append(static_cast<const char*>(data), size);
        }
    };

    ///
    string_writer writer;
    doc.save(writer);
    return std::move(writer.result);
}

const sfz::Region* sfz::Synth::getRegionView(int idx) const noexcept
{
    return (size_t)idx < regions.size() ? regions[idx].get() : nullptr;
}

const sfz::EffectBus* sfz::Synth::getEffectBusView(int idx) const noexcept
{
    return (size_t)idx < effectBuses.size() ? effectBuses[idx].get() : nullptr;
}

const sfz::RegionSet* sfz::Synth::getRegionSetView(int idx) const noexcept
{
    return (size_t)idx < sets.size() ? sets[idx].get() : nullptr;
}

const sfz::PolyphonyGroup* sfz::Synth::getPolyphonyGroupView(int idx) const noexcept
{
    return (size_t)idx < polyphonyGroups.size() ? &polyphonyGroups[idx] : nullptr;
}

const sfz::Region* sfz::Synth::getRegionById(NumericId<Region> id) const noexcept
{
    const size_t size = regions.size();

    if (size == 0 || !id.valid())
        return nullptr;

    // search a sequence of ordered identifiers with potential gaps
    size_t index = static_cast<size_t>(id.number());
    index = std::min(index, size - 1);

    while (index > 0 && regions[index]->getId().number() > id.number())
        --index;

    return (regions[index]->getId() == id) ? regions[index].get() : nullptr;
}

const sfz::Voice* sfz::Synth::getVoiceById(NumericId<Voice> id) const noexcept
{
    const size_t size = voices.size();

    if (size == 0 || !id.valid())
        return nullptr;

    // search a sequence of ordered identifiers with potential gaps
    size_t index = static_cast<size_t>(id.number());
    index = std::min(index, size - 1);

    while (index > 0 && voices[index]->getId().number() > id.number())
        --index;

    return (voices[index]->getId() == id) ? voices[index].get() : nullptr;
}

const sfz::Voice* sfz::Synth::getVoiceView(int idx) const noexcept
{
    return (size_t)idx < voices.size() ? voices[idx].get() : nullptr;
}

unsigned sfz::Synth::getNumPolyphonyGroups() const noexcept
{
    return polyphonyGroups.size();
}

const std::vector<std::string>& sfz::Synth::getUnknownOpcodes() const noexcept
{
    return unknownOpcodes;
}
size_t sfz::Synth::getNumPreloadedSamples() const noexcept
{
    return resources.filePool.getNumPreloadedSamples();
}

int sfz::Synth::getSampleQuality(ProcessMode mode)
{
    switch (mode) {
    case ProcessLive:
        return resources.synthConfig.liveSampleQuality;
    case ProcessFreewheeling:
        return resources.synthConfig.freeWheelingSampleQuality;
    default:
        SFIZZ_CHECK(false);
        return 0;
    }
}

void sfz::Synth::setSampleQuality(ProcessMode mode, int quality)
{
    SFIZZ_CHECK(quality >= 1 && quality <= 10);
    quality = clamp(quality, 1, 10);

    switch (mode) {
    case ProcessLive:
        resources.synthConfig.liveSampleQuality = quality;
        break;
    case ProcessFreewheeling:
        resources.synthConfig.freeWheelingSampleQuality = quality;
        break;
    default:
        SFIZZ_CHECK(false);
        break;
    }
}

float sfz::Synth::getVolume() const noexcept
{
    return volume;
}
void sfz::Synth::setVolume(float volume) noexcept
{
    this->volume = Default::volumeRange.clamp(volume);
}

int sfz::Synth::getNumVoices() const noexcept
{
    return numVoices;
}

void sfz::Synth::setNumVoices(int numVoices) noexcept
{
    ASSERT(numVoices > 0);
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    // fast path
    if (numVoices == this->numVoices)
        return;

    resetVoices(numVoices);
}

void sfz::Synth::resetVoices(int numVoices)
{
    voices.clear();
    voices.reserve(numVoices);

    voiceViewArray.clear();
    voiceViewArray.reserve(numVoices);

    tempPolyphonyArray.clear();
    tempPolyphonyArray.reserve(numVoices);

    for (int i = 0; i < numVoices; ++i) {
        auto voice = absl::make_unique<Voice>(i, resources);
        voice->setStateListener(this);
        voiceViewArray.push_back(voice.get());
        voices.emplace_back(std::move(voice));
    }

    for (auto& voice : voices) {
        voice->setSampleRate(this->sampleRate);
        voice->setSamplesPerBlock(this->samplesPerBlock);
    }

    this->numVoices = numVoices;

    applySettingsPerVoice();
}

void sfz::Synth::applySettingsPerVoice()
{
    for (auto& voice : voices) {
        voice->setMaxFiltersPerVoice(settingsPerVoice.maxFilters);
        voice->setMaxEQsPerVoice(settingsPerVoice.maxEQs);
        voice->setMaxLFOsPerVoice(settingsPerVoice.maxLFOs);
    }
}

void sfz::Synth::setupModMatrix()
{
    ModMatrix& mm = resources.modMatrix;

    for (const RegionPtr& region : regions) {
        for (const Region::Connection& conn : region->connections) {
            ModGenerator* gen = nullptr;

            switch (conn.source.id()) {
            case ModId::Controller:
                gen = genController.get();
                break;
            case ModId::LFO:
                gen = genLFO.get();
                break;
            default:
                DBG("[sfizz] Have unknown type of source generator");
                break;
            }

            ASSERT(gen);
            if (!gen)
                continue;

            ModMatrix::SourceId source = mm.registerSource(conn.source, *gen);
            ModMatrix::TargetId target = mm.registerTarget(conn.target);

            ASSERT(source);
            if (!source) {
                DBG("[sfizz] Failed to register modulation source");
                continue;
            }

            ASSERT(target);
            if (!target) {
                DBG("[sfizz] Failed to register modulation target");
                continue;
            }

            if (!mm.connect(source, target, conn.sourceDepth)) {
                DBG("[sfizz] Failed to connect modulation source and target");
                ASSERTFALSE;
            }
        }
    }

    mm.init();
}

void sfz::Synth::setOversamplingFactor(sfz::Oversampling factor) noexcept
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    // fast path
    if (factor == oversamplingFactor)
        return;

    for (auto& voice : voices) {

        voice->reset();
    }

    resources.filePool.emptyFileLoadingQueues();
    resources.filePool.setOversamplingFactor(factor);
    oversamplingFactor = factor;
}

sfz::Oversampling sfz::Synth::getOversamplingFactor() const noexcept
{
    return oversamplingFactor;
}

void sfz::Synth::setPreloadSize(uint32_t preloadSize) noexcept
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    // fast path
    if (preloadSize == resources.filePool.getPreloadSize())
        return;

    resources.filePool.setPreloadSize(preloadSize);
}

uint32_t sfz::Synth::getPreloadSize() const noexcept
{
    return resources.filePool.getPreloadSize();
}

void sfz::Synth::enableFreeWheeling() noexcept
{
    if (!resources.synthConfig.freeWheeling) {
        resources.synthConfig.freeWheeling = true;
        DBG("Enabling freewheeling");
    }
}
void sfz::Synth::disableFreeWheeling() noexcept
{
    if (resources.synthConfig.freeWheeling) {
        resources.synthConfig.freeWheeling = false;
        DBG("Disabling freewheeling");
    }
}

void sfz::Synth::resetAllControllers(int delay) noexcept
{
    resources.midiState.resetAllControllers(delay);

    const std::unique_lock<SpinMutex> lock { callbackGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    for (auto& voice : voices) {
        voice->registerPitchWheel(delay, 0);
        for (int cc = 0; cc < config::numCCs; ++cc)
            voice->registerCC(delay, cc, 0.0f);
    }

    for (auto& region : regions) {
        for (int cc = 0; cc < config::numCCs; ++cc)
            region->registerCC(cc, 0.0f);
    }
}

fs::file_time_type sfz::Synth::checkModificationTime()
{
    auto returnedTime = modificationTime;
    for (const auto& file : parser.getIncludedFiles()) {
        std::error_code ec;
        const auto fileTime = fs::last_write_time(file, ec);
        if (!ec && returnedTime < fileTime)
            returnedTime = fileTime;
    }
    return returnedTime;
}

bool sfz::Synth::shouldReloadFile()
{
    return (checkModificationTime() > modificationTime);
}

bool sfz::Synth::shouldReloadScala()
{
    return resources.tuning.shouldReloadScala();
}

void sfz::Synth::enableLogging(absl::string_view prefix) noexcept
{
    resources.logger.enableLogging(prefix);
}

void sfz::Synth::setLoggingPrefix(absl::string_view prefix) noexcept
{
    resources.logger.setPrefix(prefix);
}

void sfz::Synth::disableLogging() noexcept
{
    resources.logger.disableLogging();
}

void sfz::Synth::allSoundOff() noexcept
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard };

    for (auto& voice : voices)
        voice->reset();
    for (auto& effectBus : effectBuses)
        effectBus->clear();
}

void sfz::Synth::setGroupPolyphony(unsigned groupIdx, unsigned polyphony) noexcept
{
    while (polyphonyGroups.size() <= groupIdx)
        polyphonyGroups.emplace_back();

    polyphonyGroups[groupIdx].setPolyphonyLimit(polyphony);
}

std::bitset<sfz::config::numCCs> sfz::Synth::getUsedCCs() const noexcept
{
    std::bitset<sfz::config::numCCs> used;
    for (const RegionPtr& region : regions)
        updateUsedCCsFromRegion(used, *region);
    updateUsedCCsFromModulations(used, resources.modMatrix);
    return used;
}

void sfz::Synth::updateUsedCCsFromRegion(std::bitset<sfz::config::numCCs>& usedCCs, const Region& region)
{
    updateUsedCCsFromCCMap(usedCCs, region.offsetCC);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccAttack);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccRelease);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccDecay);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccDelay);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccHold);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccStart);
    updateUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccSustain);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccAttack);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccRelease);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccDecay);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccDelay);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccHold);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccStart);
    updateUsedCCsFromCCMap(usedCCs, region.pitchEG.ccSustain);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccAttack);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccRelease);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccDecay);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccDelay);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccHold);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccStart);
    updateUsedCCsFromCCMap(usedCCs, region.filterEG.ccSustain);
    updateUsedCCsFromCCMap(usedCCs, region.ccConditions);
    updateUsedCCsFromCCMap(usedCCs, region.ccTriggers);
    updateUsedCCsFromCCMap(usedCCs, region.crossfadeCCInRange);
    updateUsedCCsFromCCMap(usedCCs, region.crossfadeCCOutRange);
}

void sfz::Synth::updateUsedCCsFromModulations(std::bitset<sfz::config::numCCs>& usedCCs, const ModMatrix& mm)
{
    class CCSourceCollector : public ModMatrix::KeyVisitor {
    public:
        explicit CCSourceCollector(std::bitset<sfz::config::numCCs>& used)
            : used_(used)
        {
        }

        bool visit(const ModKey& key) override
        {
            if (key.id() == ModId::Controller)
                used_.set(key.parameters().cc);
            return true;
        }
        std::bitset<sfz::config::numCCs>& used_;
    };

    CCSourceCollector vtor(usedCCs);
    mm.visitSources(vtor);
}
