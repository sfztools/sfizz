// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SynthPrivate.h"
#include "Config.h"
#include "Debug.h"
#include "Macros.h"
#include "modulations/ModId.h"
#include "modulations/ModKey.h"
#include "modulations/ModMatrix.h"
#include "PolyphonyGroup.h"
#include "pugixml.hpp"
#include "Region.h"
#include "RegionSet.h"
#include "Resources.h"
#include "ScopedFTZ.h"
#include "StringViewHelpers.h"
#include "utility/SpinMutex.h"
#include "utility/XmlHelpers.h"
#include "Voice.h"
#include <absl/algorithm/container.h>
#include <absl/memory/memory.h>
#include <absl/strings/str_replace.h>
#include <absl/types/optional.h>
#include <absl/types/span.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <utility>

namespace sfz {

Synth::Synth()
: impl_(new Impl) // NOLINT: (paul) I don't get why clang-tidy complains here
{
}

// Need to define the dtor after Impl has been defined
Synth::~Synth()
{

}

Synth::Impl::Impl()
{
    initializeSIMDDispatchers();

    const std::lock_guard<SpinMutex> disableCallback { callbackGuard_ };
    parser_.setListener(this);
    effectFactory_.registerStandardEffectTypes();
    effectBuses_.reserve(5); // sufficient room for main and fx1-4
    resetVoices(config::numVoices);

    // modulation sources
    genController_.reset(new ControllerSource(resources_));
    genLFO_.reset(new LFOSource(voiceManager_));
    genFlexEnvelope_.reset(new FlexEnvelopeSource(voiceManager_));
    genADSREnvelope_.reset(new ADSREnvelopeSource(voiceManager_, resources_.midiState));
}

Synth::Impl::~Impl()
{
    const std::lock_guard<SpinMutex> disableCallback { callbackGuard_ };

    voiceManager_.reset();
    resources_.filePool.emptyFileLoadingQueues();
}

void Synth::Impl::onParseFullBlock(const std::string& header, const std::vector<Opcode>& members)
{
    const auto newRegionSet = [&](OpcodeScope level) {
        auto parent = currentSet_;
        while (parent && parent->getLevel() >= level)
            parent = parent->getParent();

        sets_.emplace_back(new RegionSet(parent, level));
        currentSet_ = sets_.back().get();
    };

    switch (hash(header)) {
    case hash("global"):
        globalOpcodes_ = members;
        newRegionSet(OpcodeScope::kOpcodeScopeGlobal);
        groupOpcodes_.clear();
        masterOpcodes_.clear();
        handleGlobalOpcodes(members);
        break;
    case hash("control"):
        defaultPath_ = ""; // Always reset on a new control header
        handleControlOpcodes(members);
        break;
    case hash("master"):
        masterOpcodes_ = members;
        newRegionSet(OpcodeScope::kOpcodeScopeMaster);
        groupOpcodes_.clear();
        handleMasterOpcodes(members);
        numMasters_++;
        break;
    case hash("group"):
        groupOpcodes_ = members;
        newRegionSet(OpcodeScope::kOpcodeScopeGroup);
        handleGroupOpcodes(members, masterOpcodes_);
        numGroups_++;
        break;
    case hash("region"):
        buildRegion(members);
        break;
    case hash("curve"):
        resources_.curves.addCurveFromHeader(members);
        break;
    case hash("effect"):
        handleEffectOpcodes(members);
        break;
    default:
        std::cerr << "Unknown header: " << header << '\n';
    }
}

void Synth::Impl::onParseError(const SourceRange& range, const std::string& message)
{
    const auto relativePath = range.start.filePath->lexically_relative(parser_.originalDirectory());
    std::cerr << "Parse error in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
}

void Synth::Impl::onParseWarning(const SourceRange& range, const std::string& message)
{
    const auto relativePath = range.start.filePath->lexically_relative(parser_.originalDirectory());
    std::cerr << "Parse warning in " << relativePath << " at line " << range.start.lineNumber + 1 << ": " << message << '\n';
}

void Synth::Impl::buildRegion(const std::vector<Opcode>& regionOpcodes)
{
    int regionNumber = static_cast<int>(regions_.size());
    auto lastRegion = absl::make_unique<Region>(regionNumber, resources_.midiState, defaultPath_);

    //
    auto parseOpcodes = [&](const std::vector<Opcode>& opcodes) {
        for (auto& opcode : opcodes) {
            const auto unknown = absl::c_find_if(unknownOpcodes_, [&](absl::string_view sv) { return sv.compare(opcode.opcode) == 0; });
            if (unknown != unknownOpcodes_.end()) {
                continue;
            }

            if (!lastRegion->parseOpcode(opcode))
                unknownOpcodes_.emplace_back(opcode.opcode);
        }
    };

    parseOpcodes(globalOpcodes_);
    parseOpcodes(masterOpcodes_);
    parseOpcodes(groupOpcodes_);
    parseOpcodes(regionOpcodes);

    // Create the amplitude envelope
    if (!lastRegion->flexAmpEG)
        lastRegion->getOrCreateConnection(
            ModKey::createNXYZ(ModId::AmpEG, lastRegion->id),
            ModKey::createNXYZ(ModId::MasterAmplitude, lastRegion->id)).sourceDepth = 1.0f;
    else
        lastRegion->getOrCreateConnection(
            ModKey::createNXYZ(ModId::Envelope, lastRegion->id, *lastRegion->flexAmpEG),
            ModKey::createNXYZ(ModId::MasterAmplitude, lastRegion->id)).sourceDepth = 1.0f;

    if (octaveOffset_ != 0 || noteOffset_ != 0)
        lastRegion->offsetAllKeys(octaveOffset_ * 12 + noteOffset_);

    if (lastRegion->lastKeyswitch)
        lastKeyswitchLists_[*lastRegion->lastKeyswitch].push_back(lastRegion.get());

    if (lastRegion->lastKeyswitchRange) {
        auto& range = *lastRegion->lastKeyswitchRange;
        for (uint8_t note = range.getStart(), end = range.getEnd(); note <= end; note++)
            lastKeyswitchLists_[note].push_back(lastRegion.get());
    }

    if (lastRegion->upKeyswitch)
        upKeyswitchLists_[*lastRegion->upKeyswitch].push_back(lastRegion.get());

    if (lastRegion->downKeyswitch)
        downKeyswitchLists_[*lastRegion->downKeyswitch].push_back(lastRegion.get());

    if (lastRegion->previousKeyswitch)
        previousKeyswitchLists_.push_back(lastRegion.get());

    if (lastRegion->defaultSwitch)
        currentSwitch_ = *lastRegion->defaultSwitch;

    // There was a combination of group= and polyphony= on a region, so set the group polyphony
    if (lastRegion->group != Default::group && lastRegion->polyphony != config::maxVoices) {
        voiceManager_.setGroupPolyphony(lastRegion->group, lastRegion->polyphony);
    } else {
        // Just check that there are enough polyphony groups
        voiceManager_.ensureNumPolyphonyGroups(lastRegion->group);
    }

    if (currentSet_ != nullptr) {
        lastRegion->parent = currentSet_;
        currentSet_->addRegion(lastRegion.get());
    }

    // Adapt the size of the delayed releases to avoid allocating later on
    lastRegion->delayedReleases.reserve(lastRegion->keyRange.length());

    regions_.push_back(std::move(lastRegion));
}

void Synth::Impl::clear()
{
    // Clear the background queues before removing everyone
    resources_.filePool.waitForBackgroundLoading();

    voiceManager_.reset();
    for (auto& list : lastKeyswitchLists_)
        list.clear();
    for (auto& list : downKeyswitchLists_)
        list.clear();
    for (auto& list : upKeyswitchLists_)
        list.clear();
    for (auto& list : noteActivationLists_)
        list.clear();
    for (auto& list : ccActivationLists_)
        list.clear();
    previousKeyswitchLists_.clear();

    currentSet_ = nullptr;
    sets_.clear();
    regions_.clear();
    effectBuses_.clear();
    effectBuses_.emplace_back(new EffectBus);
    effectBuses_[0]->setGainToMain(1.0);
    effectBuses_[0]->setSamplesPerBlock(samplesPerBlock_);
    effectBuses_[0]->setSampleRate(sampleRate_);
    effectBuses_[0]->clearInputs(samplesPerBlock_);
    resources_.clear();
    numGroups_ = 0;
    numMasters_ = 0;
    currentSwitch_ = absl::nullopt;
    defaultPath_ = "";
    resources_.midiState.reset();
    resources_.filePool.clear();
    resources_.filePool.setRamLoading(config::loadInRam);
    ccLabels_.clear();
    keyLabels_.clear();
    keyswitchLabels_.clear();
    globalOpcodes_.clear();
    masterOpcodes_.clear();
    groupOpcodes_.clear();
    unknownOpcodes_.clear();
    modificationTime_ = fs::file_time_type::min();

    // set default controllers
    // midistate is reset above
    fill(absl::MakeSpan(defaultCCValues_), 0.0f);
    setDefaultHdcc(7, normalizeCC(100));
    setDefaultHdcc(10, 0.5f);
    setDefaultHdcc(11, 1.0f);

    // set default controller labels
    insertPairUniquely(ccLabels_, 7, "Volume");
    insertPairUniquely(ccLabels_, 10, "Pan");
    insertPairUniquely(ccLabels_, 11, "Expression");
}

void Synth::Impl::handleMasterOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeMaster);

        switch (member.lettersOnlyHash) {
        case hash("polyphony"):
            ASSERT(currentSet_ != nullptr);
            if (auto value = readOpcode(member.value, Default::polyphonyRange))
                currentSet_->setPolyphonyLimit(*value);
            break;
        case hash("sw_default"):
            setValueFromOpcode(member, currentSwitch_, Default::keyRange);
            break;
        }
    }
}

void Synth::Impl::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeGlobal);

        switch (member.lettersOnlyHash) {
        case hash("polyphony"):
            ASSERT(currentSet_ != nullptr);
            if (auto value = readOpcode(member.value, Default::polyphonyRange))
                currentSet_->setPolyphonyLimit(*value);
            break;
        case hash("sw_default"):
            setValueFromOpcode(member, currentSwitch_, Default::keyRange);
            break;
        case hash("volume"):
            // FIXME : Probably best not to mess with this and let the host control the volume
            // setValueFromOpcode(member, volume, Default::volumeRange);
            break;
        }
    }
}

void Synth::Impl::handleGroupOpcodes(const std::vector<Opcode>& members, const std::vector<Opcode>& masterMembers)
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
            setValueFromOpcode(member, currentSwitch_, Default::keyRange);
            break;
        }
    };

    for (auto& member : masterMembers)
        parseOpcode(member);

    for (auto& member : members)
        parseOpcode(member);

    if (groupIdx && maxPolyphony) {
        voiceManager_.setGroupPolyphony(*groupIdx, *maxPolyphony);
    } else if (maxPolyphony) {
        ASSERT(currentSet_ != nullptr);
        currentSet_->setPolyphonyLimit(*maxPolyphony);
    } else if (groupIdx) {
        voiceManager_.ensureNumPolyphonyGroups(*groupIdx);
    }
}

void Synth::Impl::handleControlOpcodes(const std::vector<Opcode>& members)
{
    for (auto& rawMember : members) {
        const Opcode member = rawMember.cleanUp(kOpcodeScopeControl);

        switch (member.lettersOnlyHash) {
        case hash("set_cc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back())) {
                const auto ccValue = readOpcode(member.value, Default::midi7Range);
                if (ccValue)
                    setDefaultHdcc(member.parameters.back(), normalizeCC(*ccValue));
            }
            break;
        case hash("set_hdcc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back())) {
                const auto ccValue = readOpcode(member.value, Default::normalizedRange);
                if (ccValue)
                    setDefaultHdcc(member.parameters.back(), *ccValue);
            }
            break;
        case hash("label_cc&"):
            if (Default::ccNumberRange.containsWithEnd(member.parameters.back()))
                insertPairUniquely(ccLabels_, member.parameters.back(), std::string(member.value));
            break;
        case hash("label_key&"):
            if (member.parameters.back() <= Default::keyRange.getEnd()) {
                const auto noteNumber = static_cast<uint8_t>(member.parameters.back());
                insertPairUniquely(keyLabels_, noteNumber, std::string(member.value));
            }
            break;
        case hash("default_path"):
            defaultPath_ = absl::StrReplaceAll(trim(member.value), { { "\\", "/" } });
            DBG("Changing default sample path to " << defaultPath_);
            break;
        case hash("note_offset"):
            setValueFromOpcode(member, noteOffset_, Default::noteOffsetRange);
            break;
        case hash("octave_offset"):
            setValueFromOpcode(member, octaveOffset_, Default::octaveOffsetRange);
            break;
        case hash("hint_ram_based"):
            if (member.value == "1")
                resources_.filePool.setRamLoading(true);
            else if (member.value == "0")
                resources_.filePool.setRamLoading(false);
            else
                DBG("Unsupported value for hint_ram_based: " << member.value);
            break;
        case hash("hint_stealing"):
            switch(hash(member.value)) {
            case hash("first"):
                voiceManager_.setStealingAlgorithm(StealingAlgorithm::First);
                break;
            case hash("oldest"):
                voiceManager_.setStealingAlgorithm(StealingAlgorithm::Oldest);
                break;
            case hash("envelope_and_age"):
                voiceManager_.setStealingAlgorithm(StealingAlgorithm::EnvelopeAndAge);
                break;
            default:
                DBG("Unsupported value for hint_stealing: " << member.value);
            }
            break;
        default:
            // Unsupported control opcode
            DBG("Unsupported control opcode: " << member.opcode);
        }
    }
}

void Synth::Impl::handleEffectOpcodes(const std::vector<Opcode>& rawMembers)
{
    absl::string_view busName = "main";

    auto getOrCreateBus = [this](unsigned index) -> EffectBus& {
        if (index + 1 > effectBuses_.size())
            effectBuses_.resize(index + 1);
        EffectBusPtr& bus = effectBuses_[index];
        if (!bus) {
            bus.reset(new EffectBus);
            bus->setSampleRate(sampleRate_);
            bus->setSamplesPerBlock(samplesPerBlock_);
            bus->clearInputs(samplesPerBlock_);
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
    auto fx = effectFactory_.makeEffect(members);
    fx->setSampleRate(sampleRate_);
    fx->setSamplesPerBlock(samplesPerBlock_);
    bus.addEffect(std::move(fx));
}

bool Synth::loadSfzFile(const fs::path& file)
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    impl.clear();

    std::error_code ec;
    fs::path realFile = fs::canonical(file, ec);

    impl.parser_.parseFile(ec ? file : realFile);
    if (impl.parser_.getErrorCount() > 0)
        return false;

    if (impl.regions_.empty())
        return false;

    impl.finalizeSfzLoad();

    return true;
}

bool Synth::loadSfzString(const fs::path& path, absl::string_view text)
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    impl.clear();

    impl.parser_.parseString(path, text);
    if (impl.parser_.getErrorCount() > 0)
        return false;

    if (impl.regions_.empty())
        return false;

    impl.finalizeSfzLoad();

    return true;
}

void Synth::Impl::finalizeSfzLoad()
{
    resources_.filePool.setRootDirectory(parser_.originalDirectory());

    size_t currentRegionIndex = 0;
    size_t currentRegionCount = regions_.size();

    auto removeCurrentRegion = [this, &currentRegionIndex, &currentRegionCount]() {
        DBG("Removing the region with sample " << *regions_[currentRegionIndex]->sampleId);
        regions_.erase(regions_.begin() + currentRegionIndex);
        --currentRegionCount;
    };

    size_t maxFilters { 0 };
    size_t maxEQs { 0 };
    size_t maxLFOs { 0 };
    size_t maxFlexEGs { 0 };
    bool havePitchEG { false };
    bool haveFilterEG { false };

    FlexEGs::clearUnusedCurves();

    while (currentRegionIndex < currentRegionCount) {
        auto region = regions_[currentRegionIndex].get();

        absl::optional<FileInformation> fileInformation;

        if (!region->isGenerator()) {
            if (!resources_.filePool.checkSampleId(*region->sampleId)) {
                removeCurrentRegion();
                continue;
            }

            fileInformation = resources_.filePool.getFileInformation(*region->sampleId);
            if (!fileInformation) {
                removeCurrentRegion();
                continue;
            }

            region->hasWavetableSample = fileInformation->wavetable ||
                fileInformation->end < config::wavetableMaxFrames;
        }

        if (!region->isOscillator()) {
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

            if (region->pitchKeycenterFromSample)
                region->pitchKeycenter = fileInformation->rootKey;

            // TODO: adjust with LFO targets
            const auto maxOffset = [region]() {
                uint64_t sumOffsetCC = region->offset + region->offsetRandom;
                for (const auto& offsets : region->offsetCC)
                    sumOffsetCC += offsets.data;
                return Default::offsetCCRange.clamp(sumOffsetCC);
            }();

            if (!resources_.filePool.preloadFile(*region->sampleId, maxOffset))
                removeCurrentRegion();
        }
        else if (!region->isGenerator()) {
            if (!resources_.wavePool.createFileWave(resources_.filePool, std::string(region->sampleId->filename()))) {
                removeCurrentRegion();
                continue;
            }
        }

        if (region->lastKeyswitch) {
            if (currentSwitch_)
                region->keySwitched = (*currentSwitch_ == *region->lastKeyswitch);

            if (region->keyswitchLabel)
                insertPairUniquely(keyswitchLabels_, *region->lastKeyswitch, *region->keyswitchLabel);
        }

        if (region->lastKeyswitchRange) {
            auto& range = *region->lastKeyswitchRange;
            if (currentSwitch_)
                region->keySwitched = range.containsWithEnd(*currentSwitch_);

            if (region->keyswitchLabel) {
                for (uint8_t note = range.getStart(), end = range.getEnd(); note <= end; note++)
                    insertPairUniquely(keyswitchLabels_, note, *region->keyswitchLabel);
            }
        }

        for (auto note = 0; note < 128; note++) {
            if (region->keyRange.containsWithEnd(note))
                noteActivationLists_[note].push_back(region);
        }

        for (int cc = 0; cc < config::numCCs; cc++) {
            if (region->ccTriggers.contains(cc)
                || region->ccConditions.contains(cc)
                || (cc == region->sustainCC && region->trigger == SfzTrigger::release))
                ccActivationLists_[cc].push_back(region);
        }

        // Defaults
        for (int cc = 0; cc < config::numCCs; cc++) {
            region->registerCC(cc, resources_.midiState.getCCValue(cc));
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
        maxFlexEGs = max(maxFlexEGs, region->flexEGs.size());
        havePitchEG = havePitchEG || region->pitchEG != absl::nullopt;
        haveFilterEG = haveFilterEG || region->filterEG != absl::nullopt;

        ++currentRegionIndex;
    }
    if (currentRegionCount < regions_.size()) {
        DBG("Removing " << (regions_.size() - currentRegionCount)
            << " out of " << regions_.size() << " regions");
    }
    regions_.resize(currentRegionCount);

    // collect all CCs used in regions, with matrix not yet connected
    std::bitset<config::numCCs> usedCCs;
    for (const RegionPtr& regionPtr : regions_) {
        const Region& region = *regionPtr;
        collectUsedCCsFromRegion(usedCCs, region);
        for (const Region::Connection& connection : region.connections) {
            if (connection.source.id() == ModId::Controller)
                usedCCs.set(connection.source.parameters().cc);
        }
    }
    // connect default controllers, except if these CC are already used
    for (const RegionPtr& regionPtr : regions_) {
        Region& region = *regionPtr;
        constexpr unsigned defaultSmoothness = 10;
        if (!usedCCs.test(7)) {
            region.getOrCreateConnection(
                ModKey::createCC(7, 4, defaultSmoothness, 0),
                ModKey::createNXYZ(ModId::Amplitude, region.id)).sourceDepth = 100.0f;
        }
        if (!usedCCs.test(10)) {
            region.getOrCreateConnection(
                ModKey::createCC(10, 1, defaultSmoothness, 0),
                ModKey::createNXYZ(ModId::Pan, region.id)).sourceDepth = 100.0f;
        }
        if (!usedCCs.test(11)) {
            region.getOrCreateConnection(
                ModKey::createCC(11, 4, defaultSmoothness, 0),
                ModKey::createNXYZ(ModId::Amplitude, region.id)).sourceDepth = 100.0f;
        }
    }

    modificationTime_ = checkModificationTime();

    settingsPerVoice_.maxFilters = maxFilters;
    settingsPerVoice_.maxEQs = maxEQs;
    settingsPerVoice_.maxLFOs = maxLFOs;
    settingsPerVoice_.maxFlexEGs = maxFlexEGs;
    settingsPerVoice_.havePitchEG = havePitchEG;
    settingsPerVoice_.haveFilterEG = haveFilterEG;

    applySettingsPerVoice();

    setupModMatrix();

    // cache the set of used CCs for future access
    currentUsedCCs_ = collectAllUsedCCs();
}

bool Synth::loadScalaFile(const fs::path& path)
{
    Impl& impl = *impl_;
    return impl.resources_.tuning.loadScalaFile(path);
}

bool Synth::loadScalaString(const std::string& text)
{
    Impl& impl = *impl_;
    return impl.resources_.tuning.loadScalaString(text);
}

void Synth::setScalaRootKey(int rootKey)
{
    Impl& impl = *impl_;
    impl.resources_.tuning.setScalaRootKey(rootKey);
}

int Synth::getScalaRootKey() const
{
    Impl& impl = *impl_;
    return impl.resources_.tuning.getScalaRootKey();
}

void Synth::setTuningFrequency(float frequency)
{
    Impl& impl = *impl_;
    impl.resources_.tuning.setTuningFrequency(frequency);
}

float Synth::getTuningFrequency() const
{
    Impl& impl = *impl_;
    return impl.resources_.tuning.getTuningFrequency();
}

void Synth::loadStretchTuningByRatio(float ratio)
{
    Impl& impl = *impl_;
    SFIZZ_CHECK(ratio >= 0.0f && ratio <= 1.0f);
    ratio = clamp(ratio, 0.0f, 1.0f);

    if (ratio > 0.0f)
        impl.resources_.stretch = StretchTuning::createRailsbackFromRatio(ratio);
    else
        impl.resources_.stretch.reset();
}

int Synth::getNumActiveVoices() const noexcept
{
    Impl& impl = *impl_;
    return static_cast<int>(impl.voiceManager_.getNumActiveVoices());
}

void Synth::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    Impl& impl = *impl_;
    ASSERT(samplesPerBlock <= config::maxBlockSize);

    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    impl.samplesPerBlock_ = samplesPerBlock;
    for (auto& voice : impl.voiceManager_)
        voice.setSamplesPerBlock(samplesPerBlock);

    impl.resources_.setSamplesPerBlock(samplesPerBlock);

    for (auto& bus : impl.effectBuses_) {
        if (bus)
            bus->setSamplesPerBlock(samplesPerBlock);
    }
}

void Synth::setSampleRate(float sampleRate) noexcept
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    impl.sampleRate_ = sampleRate;
    for (auto& voice : impl.voiceManager_)
        voice.setSampleRate(sampleRate);

    impl.resources_.setSampleRate(sampleRate);

    for (auto& bus : impl.effectBuses_) {
        if (bus)
            bus->setSampleRate(sampleRate);
    }
}

void Synth::renderBlock(AudioSpan<float> buffer) noexcept
{
    Impl& impl = *impl_;
    ScopedFTZ ftz;
    CallbackBreakdown callbackBreakdown;

    { // Silence buffer
        ScopedTiming logger { callbackBreakdown.renderMethod };
        buffer.fill(0.0f);
    }

    if (impl.resources_.synthConfig.freeWheeling)
        impl.resources_.filePool.waitForBackgroundLoading();

    const auto now = std::chrono::high_resolution_clock::now();
    const auto timeSinceLastCollection =
        std::chrono::duration_cast<std::chrono::seconds>(now - impl.lastGarbageCollection_);

    if (timeSinceLastCollection.count() > config::fileClearingPeriod) {
        impl.lastGarbageCollection_ = now;
        impl.resources_.filePool.triggerGarbageCollection();
    }

    const std::unique_lock<SpinMutex> lock { impl.callbackGuard_, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    size_t numFrames = buffer.getNumFrames();
    auto tempSpan = impl.resources_.bufferPool.getStereoBuffer(numFrames);
    auto tempMixSpan = impl.resources_.bufferPool.getStereoBuffer(numFrames);
    auto rampSpan = impl.resources_.bufferPool.getBuffer(numFrames);
    if (!tempSpan || !tempMixSpan || !rampSpan) {
        DBG("[sfizz] Could not get a temporary buffer; exiting callback... ");
        return;
    }

    ModMatrix& mm = impl.resources_.modMatrix;
    mm.beginCycle(numFrames);

    { // Clear effect busses
        ScopedTiming logger { callbackBreakdown.effects };
        for (auto& bus : impl.effectBuses_) {
            if (bus)
                bus->clearInputs(numFrames);
        }
    }

    { // Main render block
        ScopedTiming logger { callbackBreakdown.renderMethod, ScopedTiming::Operation::addToDuration };
        tempMixSpan->fill(0.0f);

        for (auto& voice : impl.voiceManager_) {
            if (voice.isFree())
                continue;

            mm.beginVoice(voice.getId(), voice.getRegion()->getId(), voice.getTriggerEvent().value);

            const Region* region = voice.getRegion();
            ASSERT(region != nullptr);

            voice.renderBlock(*tempSpan);
            for (size_t i = 0, n = impl.effectBuses_.size(); i < n; ++i) {
                if (auto& bus = impl.effectBuses_[i]) {
                    float addGain = region->getGainToEffectBus(i);
                    bus->addToInputs(*tempSpan, addGain, numFrames);
                }
            }
            callbackBreakdown.data += voice.getLastDataDuration();
            callbackBreakdown.amplitude += voice.getLastAmplitudeDuration();
            callbackBreakdown.filters += voice.getLastFilterDuration();
            callbackBreakdown.panning += voice.getLastPanningDuration();

            mm.endVoice();

            if (voice.toBeCleanedUp())
                voice.reset();
        }
    }

    { // Apply effect buses
        // -- note(jpc) there is always a "main" bus which is initially empty.
        //    without any <effect>, the signal is just going to flow through it.
        ScopedTiming logger { callbackBreakdown.effects, ScopedTiming::Operation::addToDuration };

        for (auto& bus : impl.effectBuses_) {
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
    buffer.applyGain(db2mag(impl.volume_));

    // Perform any remaining modulators
    mm.endCycle();

    { // Clear events and advance midi time
        ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
        impl.resources_.midiState.advanceTime(buffer.getNumFrames());
    }

    callbackBreakdown.dispatch = impl.dispatchDuration_;
    impl.resources_.logger.logCallbackTime(
        callbackBreakdown, impl.voiceManager_.getNumActiveVoices(), numFrames);

    // Reset the dispatch counter
    impl.dispatchDuration_ = Duration(0);

    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
}

void Synth::noteOn(int delay, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    Impl& impl = *impl_;
    const auto normalizedVelocity = normalizeVelocity(velocity);
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
    impl.resources_.midiState.noteOnEvent(delay, noteNumber, normalizedVelocity);

    const std::unique_lock<SpinMutex> lock { impl.callbackGuard_, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    impl.noteOnDispatch(delay, noteNumber, normalizedVelocity);
}

void Synth::noteOff(int delay, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    UNUSED(velocity);
    Impl& impl = *impl_;
    const auto normalizedVelocity = normalizeVelocity(velocity);
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
    impl.resources_.midiState.noteOffEvent(delay, noteNumber, normalizedVelocity);

    const std::unique_lock<SpinMutex> lock { impl.callbackGuard_, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    // FIXME: Some keyboards (e.g. Casio PX5S) can send a real note-off velocity. In this case, do we have a
    // way in sfz to specify that a release trigger should NOT use the note-on velocity?
    // auto replacedVelocity = (velocity == 0 ? getNoteVelocity(noteNumber) : velocity);
    const auto replacedVelocity = impl.resources_.midiState.getNoteVelocity(noteNumber);

    for (auto& voice : impl.voiceManager_)
        voice.registerNoteOff(delay, noteNumber, replacedVelocity);

    impl.noteOffDispatch(delay, noteNumber, replacedVelocity);
}

void Synth::Impl::startVoice(Region* region, int delay, const TriggerEvent& triggerEvent, SisterVoiceRingBuilder& ring) noexcept
{
    voiceManager_.checkPolyphony(region, delay, triggerEvent);
    Voice* selectedVoice = voiceManager_.findFreeVoice();
    if (selectedVoice == nullptr)
        return;

    ASSERT(selectedVoice->isFree());
    selectedVoice->startVoice(region, delay, triggerEvent);
    ring.addVoiceToRing(selectedVoice);
}

void Synth::Impl::noteOffDispatch(int delay, int noteNumber, float velocity) noexcept
{
    const auto randValue = randNoteDistribution_(Random::randomGenerator);
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::NoteOff, noteNumber, velocity };

    for (auto& region : upKeyswitchLists_[noteNumber])
        region->keySwitched = true;

    for (auto& region : downKeyswitchLists_[noteNumber])
        region->keySwitched = false;

    for (auto& region : noteActivationLists_[noteNumber]) {
        if (region->registerNoteOff(noteNumber, velocity, randValue)) {
            if (region->trigger == SfzTrigger::release && !region->rtDead && !voiceManager_.playingAttackVoice(region))
                continue;

            startVoice(region, delay, triggerEvent, ring);
        }
    }
}

void Synth::Impl::noteOnDispatch(int delay, int noteNumber, float velocity) noexcept
{
    const auto randValue = randNoteDistribution_(Random::randomGenerator);
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::NoteOn, noteNumber, velocity };

    if (!lastKeyswitchLists_[noteNumber].empty()) {
        if (currentSwitch_ && *currentSwitch_ != noteNumber) {
            for (auto& region : lastKeyswitchLists_[*currentSwitch_])
                region->keySwitched = false;
        }
        currentSwitch_ = noteNumber;
    }

    for (auto& region : lastKeyswitchLists_[noteNumber])
        region->keySwitched = true;

    for (auto& region : upKeyswitchLists_[noteNumber])
        region->keySwitched = false;

    for (auto& region : downKeyswitchLists_[noteNumber])
        region->keySwitched = true;

    for (auto& region : noteActivationLists_[noteNumber]) {
        if (region->registerNoteOn(noteNumber, velocity, randValue)) {
            for (auto& voice : voiceManager_) {
                if (voice.checkOffGroup(region, delay, noteNumber)) {
                    const TriggerEvent& event = voice.getTriggerEvent();
                    noteOffDispatch(delay, event.number, event.value);
                }
            }

            startVoice(region, delay, triggerEvent, ring);
        }
    }

    for (auto& region : previousKeyswitchLists_)
        region->previousKeySwitched = (*region->previousKeyswitch == noteNumber);
}

void Synth::Impl::startDelayedReleaseVoices(Region* region, int delay, SisterVoiceRingBuilder& ring) noexcept
{
    if (!region->rtDead && !voiceManager_.playingAttackVoice(region)) {
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


void Synth::cc(int delay, int ccNumber, uint8_t ccValue) noexcept
{
    const auto normalizedCC = normalizeCC(ccValue);
    hdcc(delay, ccNumber, normalizedCC);
}

void Synth::Impl::ccDispatch(int delay, int ccNumber, float value) noexcept
{
    SisterVoiceRingBuilder ring;
    const TriggerEvent triggerEvent { TriggerEventType::CC, ccNumber, value };
    for (auto& region : ccActivationLists_[ccNumber]) {
        if (ccNumber == region->sustainCC)
            startDelayedReleaseVoices(region, delay, ring);

        if (region->registerCC(ccNumber, value))
            startVoice(region, delay, triggerEvent, ring);
    }
}

void Synth::hdcc(int delay, int ccNumber, float normValue) noexcept
{
    Impl& impl = *impl_;
    impl.performHdcc(delay, ccNumber, normValue, true);
}

void Synth::automateHdcc(int delay, int ccNumber, float normValue) noexcept
{
    Impl& impl = *impl_;
    impl.performHdcc(delay, ccNumber, normValue, false);
}

void Synth::Impl::performHdcc(int delay, int ccNumber, float normValue, bool asMidi) noexcept
{
    ASSERT(ccNumber < config::numCCs);
    ASSERT(ccNumber >= 0);

    ScopedTiming logger { dispatchDuration_, ScopedTiming::Operation::addToDuration };
    resources_.midiState.ccEvent(delay, ccNumber, normValue);

    const std::unique_lock<SpinMutex> lock { callbackGuard_, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    if (asMidi) {
        if (ccNumber == config::resetCC) {
            resetAllControllers(delay);
            return;
        }

        if (ccNumber == config::allNotesOffCC || ccNumber == config::allSoundOffCC) {
            for (auto& voice : voiceManager_)
                voice.reset();
            resources_.midiState.allNotesOff(delay);
            return;
        }
    }

    for (auto& voice : voiceManager_)
        voice.registerCC(delay, ccNumber, normValue);

    ccDispatch(delay, ccNumber, normValue);
}

void Synth::Impl::setDefaultHdcc(int ccNumber, float value)
{
    ASSERT(ccNumber >= 0);
    ASSERT(ccNumber < config::numCCs);
    defaultCCValues_[ccNumber] = value;
    resources_.midiState.ccEvent(0, ccNumber, value);
}

float Synth::getHdcc(int ccNumber)
{
    ASSERT(ccNumber >= 0);
    ASSERT(ccNumber < config::numCCs);
    Impl& impl = *impl_;
    return impl.resources_.midiState.getCCValue(ccNumber);
}

float Synth::getDefaultHdcc(int ccNumber)
{
    ASSERT(ccNumber >= 0);
    ASSERT(ccNumber < config::numCCs);
    Impl& impl = *impl_;
    return impl.defaultCCValues_[ccNumber];
}

void Synth::pitchWheel(int delay, int pitch) noexcept
{
    ASSERT(pitch <= 8192);
    ASSERT(pitch >= -8192);
    Impl& impl = *impl_;
    const auto normalizedPitch = normalizeBend(float(pitch));

    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
    impl.resources_.midiState.pitchBendEvent(delay, normalizedPitch);

    for (auto& region : impl.regions_) {
        region->registerPitchWheel(normalizedPitch);
    }

    for (auto& voice : impl.voiceManager_) {
        voice.registerPitchWheel(delay, normalizedPitch);
    }
}
void Synth::aftertouch(int /* delay */, uint8_t /* aftertouch */) noexcept
{
    Impl& impl = *impl_;
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
}
void Synth::tempo(int /* delay */, float /* secondsPerQuarter */) noexcept
{
    Impl& impl = *impl_;
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };
}
void Synth::timeSignature(int delay, int beatsPerBar, int beatUnit)
{
    Impl& impl = *impl_;
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)beatsPerBar;
    (void)beatUnit;
}
void Synth::timePosition(int delay, int bar, float barBeat)
{
    Impl& impl = *impl_;
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)bar;
    (void)barBeat;
}
void Synth::playbackState(int delay, int playbackState)
{
    Impl& impl = *impl_;
    ScopedTiming logger { impl.dispatchDuration_, ScopedTiming::Operation::addToDuration };

    (void)delay;
    (void)playbackState;
}

int Synth::getNumRegions() const noexcept
{
    Impl& impl = *impl_;
    return static_cast<int>(impl.regions_.size());
}
int Synth::getNumGroups() const noexcept
{
    Impl& impl = *impl_;
    return impl.numGroups_;
}
int Synth::getNumMasters() const noexcept
{
    Impl& impl = *impl_;
    return impl.numMasters_;
}
int Synth::getNumCurves() const noexcept
{
    Impl& impl = *impl_;
    return static_cast<int>(impl.resources_.curves.getNumCurves());
}

std::string Synth::exportMidnam(absl::string_view model) const
{
    Impl& impl = *impl_;
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
        for (const auto& pair : impl.ccLabels_) {
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
        for (const auto& pair : impl.keyswitchLabels_) {
            pugi::xml_node nn = nnl.append_child("Note");
            nn.append_attribute("Number").set_value(std::to_string(pair.first).c_str());
            nn.append_attribute("Name").set_value(pair.second.c_str());
        }
        for (const auto& pair : impl.keyLabels_) {
            pugi::xml_node nn = nnl.append_child("Note");
            nn.append_attribute("Number").set_value(std::to_string(pair.first).c_str());
            nn.append_attribute("Name").set_value(pair.second.c_str());
        }
    }

    string_xml_writer writer;
    doc.save(writer);
    return std::move(writer.str());
}

const Region* Synth::getRegionView(int idx) const noexcept
{
    Impl& impl = *impl_;
    return (size_t)idx < impl.regions_.size() ? impl.regions_[idx].get() : nullptr;
}

const EffectBus* Synth::getEffectBusView(int idx) const noexcept
{
    Impl& impl = *impl_;
    return (size_t)idx < impl.effectBuses_.size() ? impl.effectBuses_[idx].get() : nullptr;
}

const RegionSet* Synth::getRegionSetView(int idx) const noexcept
{
    Impl& impl = *impl_;
    return (size_t)idx < impl.sets_.size() ? impl.sets_[idx].get() : nullptr;
}

const PolyphonyGroup* Synth::getPolyphonyGroupView(int idx) const noexcept
{
    Impl& impl = *impl_;
    return impl.voiceManager_.getPolyphonyGroupView(idx);
}

const Region* Synth::getRegionById(NumericId<Region> id) const noexcept
{
    Impl& impl = *impl_;
    const size_t size = impl.regions_.size();

    if (size == 0 || !id.valid())
        return nullptr;

    // search a sequence of ordered identifiers with potential gaps
    size_t index = static_cast<size_t>(id.number());
    index = std::min(index, size - 1);

    while (index > 0 && impl.regions_[index]->getId().number() > id.number())
        --index;

    return (impl.regions_[index]->getId() == id) ? impl.regions_[index].get() : nullptr;
}

const Voice* Synth::getVoiceView(int idx) const noexcept
{
    Impl& impl = *impl_;
    return idx < impl.numVoices_ ? &impl.voiceManager_[idx] : nullptr;
}

unsigned Synth::getNumPolyphonyGroups() const noexcept
{
    Impl& impl = *impl_;
    return impl.voiceManager_.getNumPolyphonyGroups();
}

const std::vector<std::string>& Synth::getUnknownOpcodes() const noexcept
{
    Impl& impl = *impl_;
    return impl.unknownOpcodes_;
}
size_t Synth::getNumPreloadedSamples() const noexcept
{
    Impl& impl = *impl_;
    return impl.resources_.filePool.getNumPreloadedSamples();
}

int Synth::getSampleQuality(ProcessMode mode)
{
    Impl& impl = *impl_;
    switch (mode) {
    case ProcessLive:
        return impl.resources_.synthConfig.liveSampleQuality;
    case ProcessFreewheeling:
        return impl.resources_.synthConfig.freeWheelingSampleQuality;
    default:
        SFIZZ_CHECK(false);
        return 0;
    }
}

void Synth::setSampleQuality(ProcessMode mode, int quality)
{
    SFIZZ_CHECK(quality >= 1 && quality <= 10);
    Impl& impl = *impl_;
    quality = clamp(quality, 1, 10);

    switch (mode) {
    case ProcessLive:
        impl.resources_.synthConfig.liveSampleQuality = quality;
        break;
    case ProcessFreewheeling:
        impl.resources_.synthConfig.freeWheelingSampleQuality = quality;
        break;
    default:
        SFIZZ_CHECK(false);
        break;
    }
}

float Synth::getVolume() const noexcept
{
    Impl& impl = *impl_;
    return impl.volume_;
}
void Synth::setVolume(float volume) noexcept
{
    Impl& impl = *impl_;
    impl.volume_ = Default::volumeRange.clamp(volume);
}

int Synth::getNumVoices() const noexcept
{
    Impl& impl = *impl_;
    return impl.numVoices_;
}

void Synth::setNumVoices(int numVoices) noexcept
{
    ASSERT(numVoices > 0);
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    // fast path
    if (numVoices == impl.numVoices_)
        return;

    impl.resetVoices(numVoices);
}

void Synth::Impl::resetVoices(int numVoices)
{
    numVoices_ = numVoices;

    for (auto& set : sets_)
        set->removeAllVoices();

    voiceManager_.requireNumVoices(numVoices_, resources_);

    for (auto& voice : voiceManager_) {
        voice.setSampleRate(this->sampleRate_);
        voice.setSamplesPerBlock(this->samplesPerBlock_);
    }

    applySettingsPerVoice();
}

void Synth::Impl::applySettingsPerVoice()
{
    for (auto& voice : voiceManager_) {
        voice.setMaxFiltersPerVoice(settingsPerVoice_.maxFilters);
        voice.setMaxEQsPerVoice(settingsPerVoice_.maxEQs);
        voice.setMaxLFOsPerVoice(settingsPerVoice_.maxLFOs);
        voice.setMaxFlexEGsPerVoice(settingsPerVoice_.maxFlexEGs);
        voice.setPitchEGEnabledPerVoice(settingsPerVoice_.havePitchEG);
        voice.setFilterEGEnabledPerVoice(settingsPerVoice_.haveFilterEG);
    }
}

void Synth::Impl::setupModMatrix()
{
    ModMatrix& mm = resources_.modMatrix;

    for (const RegionPtr& region : regions_) {
        for (const Region::Connection& conn : region->connections) {
            ModGenerator* gen = nullptr;

            ModKey sourceKey = conn.source;
            ModKey targetKey = conn.target;

            // normalize the stepcc to 0-1
            if (sourceKey.id() == ModId::Controller) {
                ModKey::Parameters p = sourceKey.parameters();
                p.step = (conn.sourceDepth == 0.0f) ? 0.0f :
                    (p.step / conn.sourceDepth);
                sourceKey = ModKey::createCC(p.cc, p.curve, p.smooth, p.step);
            }

            switch (sourceKey.id()) {
            case ModId::Controller:
                gen = genController_.get();
                break;
            case ModId::LFO:
                gen = genLFO_.get();
                break;
            case ModId::Envelope:
                gen = genFlexEnvelope_.get();
                break;
            case ModId::AmpEG:
            case ModId::PitchEG:
            case ModId::FilEG:
                gen = genADSREnvelope_.get();
                break;
            default:
                DBG("[sfizz] Have unknown type of source generator");
                break;
            }

            ASSERT(gen);
            if (!gen)
                continue;

            ModMatrix::SourceId source = mm.registerSource(sourceKey, *gen);
            ModMatrix::TargetId target = mm.registerTarget(targetKey);

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

            if (!mm.connect(source, target, conn.sourceDepth, conn.velToDepth)) {
                DBG("[sfizz] Failed to connect modulation source and target");
                ASSERTFALSE;
            }
        }
    }

    mm.init();
}

void Synth::setOversamplingFactor(Oversampling factor) noexcept
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    // fast path
    if (factor == impl.oversamplingFactor_)
        return;

    for (auto& voice : impl.voiceManager_)
        voice.reset();

    impl.resources_.filePool.emptyFileLoadingQueues();
    impl.resources_.filePool.setOversamplingFactor(factor);
    impl.oversamplingFactor_ = factor;
}

Oversampling Synth::getOversamplingFactor() const noexcept
{
    Impl& impl = *impl_;
    return impl.oversamplingFactor_;
}

void Synth::setPreloadSize(uint32_t preloadSize) noexcept
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    // fast path
    if (preloadSize == impl.resources_.filePool.getPreloadSize())
        return;

    impl.resources_.filePool.setPreloadSize(preloadSize);
}

uint32_t Synth::getPreloadSize() const noexcept
{
    Impl& impl = *impl_;
    return impl.resources_.filePool.getPreloadSize();
}

void Synth::enableFreeWheeling() noexcept
{
    Impl& impl = *impl_;
    if (!impl.resources_.synthConfig.freeWheeling) {
        impl.resources_.synthConfig.freeWheeling = true;
        DBG("Enabling freewheeling");
    }
}
void Synth::disableFreeWheeling() noexcept
{
    Impl& impl = *impl_;
    if (impl.resources_.synthConfig.freeWheeling) {
        impl.resources_.synthConfig.freeWheeling = false;
        DBG("Disabling freewheeling");
    }
}

void Synth::Impl::resetAllControllers(int delay) noexcept
{
    resources_.midiState.resetAllControllers(delay);

    const std::unique_lock<SpinMutex> lock { callbackGuard_, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    for (auto& voice : voiceManager_) {
        voice.registerPitchWheel(delay, 0);
        for (int cc = 0; cc < config::numCCs; ++cc)
            voice.registerCC(delay, cc, 0.0f);
    }

    for (auto& region : regions_) {
        for (int cc = 0; cc < config::numCCs; ++cc)
            region->registerCC(cc, 0.0f);
    }
}

fs::file_time_type Synth::Impl::checkModificationTime()
{
    auto returnedTime = modificationTime_;
    for (const auto& file : parser_.getIncludedFiles()) {
        std::error_code ec;
        const auto fileTime = fs::last_write_time(file, ec);
        if (!ec && returnedTime < fileTime)
            returnedTime = fileTime;
    }
    return returnedTime;
}

bool Synth::shouldReloadFile()
{
    Impl& impl = *impl_;
    return (impl.checkModificationTime() > impl.modificationTime_);
}

bool Synth::shouldReloadScala()
{
    Impl& impl = *impl_;
    return impl.resources_.tuning.shouldReloadScala();
}

void Synth::enableLogging(absl::string_view prefix) noexcept
{
    Impl& impl = *impl_;
    impl.resources_.logger.enableLogging(prefix);
}

void Synth::setLoggingPrefix(absl::string_view prefix) noexcept
{
    Impl& impl = *impl_;
    impl.resources_.logger.setPrefix(prefix);
}

void Synth::disableLogging() noexcept
{
    Impl& impl = *impl_;
    impl.resources_.logger.disableLogging();
}

void Synth::allSoundOff() noexcept
{
    Impl& impl = *impl_;
    const std::lock_guard<SpinMutex> disableCallback { impl.callbackGuard_ };

    for (auto& voice : impl.voiceManager_)
        voice.reset();
    for (auto& effectBus : impl.effectBuses_)
        effectBus->clear();
}

const std::bitset<config::numCCs>& Synth::getUsedCCs() const noexcept
{
    Impl& impl = *impl_;
    return impl.currentUsedCCs_;
}

void sfz::Synth::setBroadcastCallback(sfizz_receive_t* broadcast, void* data)
{
    Impl& impl = *impl_;
    impl.broadcastReceiver = broadcast;
    impl.broadcastData = data;
}

void Synth::Impl::collectUsedCCsFromRegion(std::bitset<config::numCCs>& usedCCs, const Region& region)
{
    collectUsedCCsFromCCMap(usedCCs, region.offsetCC);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccAttack);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccRelease);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccDecay);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccDelay);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccHold);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccStart);
    collectUsedCCsFromCCMap(usedCCs, region.amplitudeEG.ccSustain);
    if (region.pitchEG) {
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccAttack);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccRelease);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccDecay);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccDelay);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccHold);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccStart);
        collectUsedCCsFromCCMap(usedCCs, region.pitchEG->ccSustain);
    }
    if (region.filterEG) {
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccAttack);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccRelease);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccDecay);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccDelay);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccHold);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccStart);
        collectUsedCCsFromCCMap(usedCCs, region.filterEG->ccSustain);
    }
    collectUsedCCsFromCCMap(usedCCs, region.ccConditions);
    collectUsedCCsFromCCMap(usedCCs, region.ccTriggers);
    collectUsedCCsFromCCMap(usedCCs, region.crossfadeCCInRange);
    collectUsedCCsFromCCMap(usedCCs, region.crossfadeCCOutRange);
}

void Synth::Impl::collectUsedCCsFromModulations(std::bitset<config::numCCs>& usedCCs, const ModMatrix& mm)
{
    class CCSourceCollector : public ModMatrix::KeyVisitor {
    public:
        explicit CCSourceCollector(std::bitset<config::numCCs>& used)
            : used_(used)
        {
        }

        bool visit(const ModKey& key) override
        {
            if (key.id() == ModId::Controller)
                used_.set(key.parameters().cc);
            return true;
        }
        std::bitset<config::numCCs>& used_;
    };

    CCSourceCollector vtor(usedCCs);
    mm.visitSources(vtor);
}

std::bitset<config::numCCs> Synth::Impl::collectAllUsedCCs()
{
    std::bitset<config::numCCs> used;
    for (const Impl::RegionPtr& region : regions_)
        collectUsedCCsFromRegion(used, *region);
    collectUsedCCsFromModulations(used, resources_.modMatrix);
    return used;
}

Parser& Synth::getParser() noexcept
{
    Impl& impl = *impl_;
    return impl.parser_;
}

const Parser& Synth::getParser() const noexcept
{
    Impl& impl = *impl_;
    return impl.parser_;
}

const std::vector<NoteNamePair>& Synth::getKeyLabels() const noexcept
{
    Impl& impl = *impl_;
    return impl.keyLabels_;
}

const std::vector<CCNamePair>& Synth::getCCLabels() const noexcept
{
    Impl& impl = *impl_;
    return impl.ccLabels_;
}

Resources& Synth::getResources() noexcept
{
    Impl& impl = *impl_;
    return impl.resources_;
}

const Resources& Synth::getResources() const noexcept
{
    Impl& impl = *impl_;
    return impl.resources_;
}

} // namespace sfz
