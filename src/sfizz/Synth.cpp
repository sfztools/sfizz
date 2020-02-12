// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Synth.h"
#include "AtomicGuard.h"
#include "Config.h"
#include "Debug.h"
#include "MidiState.h"
#include "ScopedFTZ.h"
#include "StringViewHelpers.h"
#include "absl/algorithm/container.h"
#include "absl/strings/str_replace.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>
using namespace std::literals;

sfz::Synth::Synth()
{
    resetVoices(this->numVoices);
}

sfz::Synth::Synth(int numVoices)
{
    resetVoices(numVoices);
}

sfz::Synth::~Synth()
{
    AtomicDisabler callbackDisabler { canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    for (auto& voice: voices)
        voice->reset();

    resources.filePool.emptyFileLoadingQueues();
}

void sfz::Synth::callback(absl::string_view header, const std::vector<Opcode>& members)
{
    switch (hash(header)) {
    case hash("global"):
        globalOpcodes = members;
        handleGlobalOpcodes(members);
        break;
    case hash("control"):
        defaultPath = ""; // Always reset on a new control header
        handleControlOpcodes(members);
        break;
    case hash("master"):
        masterOpcodes = members;
        numMasters++;
        break;
    case hash("group"):
        groupOpcodes = members;
        numGroups++;
        break;
    case hash("region"):
        buildRegion(members);
        break;
    case hash("curve"):
        // TODO: implement curves
        numCurves++;
        break;
    case hash("effect"):
        // TODO: implement effects
        break;
    default:
        std::cerr << "Unknown header: " << header << '\n';
    }
}

void sfz::Synth::buildRegion(const std::vector<Opcode>& regionOpcodes)
{
    auto lastRegion = std::make_unique<Region>(resources.midiState, defaultPath);

    auto parseOpcodes = [&](const auto& opcodes) {
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

    regions.push_back(std::move(lastRegion));
}

void sfz::Synth::clear()
{
    AtomicDisabler callbackDisabler { canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    for (auto &voice: voices)
        voice->reset();
    for (auto& list: noteActivationLists)
        list.clear();
    for (auto& list: ccActivationLists)
        list.clear();
    regions.clear();
    resources.filePool.clear();
    resources.logger.clear();
    numGroups = 0;
    numMasters = 0;
    numCurves = 0;
    fileTicket = -1;
    defaultSwitch = absl::nullopt;
    defaultPath = "";
    resources.midiState.reset();
    ccNames.clear();
    globalOpcodes.clear();
    masterOpcodes.clear();
    groupOpcodes.clear();
    unknownOpcodes.clear();
    modificationTime = fs::file_time_type::min();
}

void sfz::Synth::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member : members) {
        switch (member.lettersOnlyHash) {
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

void sfz::Synth::handleControlOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member : members) {
        const auto backParameter = member.backParameter();
        switch (member.lettersOnlyHash) {
        case hash("Set_cc"):
            [[fallthrough]];
        case hash("set_cc"):
            if (backParameter && Default::ccNumberRange.containsWithEnd(*backParameter)) {
                const auto ccValue = readOpcode(member.value, Default::ccValueRange).value_or(0);
                resources.midiState.ccEvent(*backParameter, ccValue);
            }
            break;
        case hash("Label_cc"):
            [[fallthrough]];
        case hash("label_cc"):
            if (backParameter && Default::ccNumberRange.containsWithEnd(*backParameter))
                ccNames.emplace_back(*backParameter, std::string(member.value));
            break;
        case hash("Default_path"):
            [[fallthrough]];
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

void addEndpointsToVelocityCurve(sfz::Region& region)
{
    if (region.velocityPoints.size() > 0) {
        absl::c_sort(region.velocityPoints, [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
        if (region.ampVeltrack > 0) {
            if (region.velocityPoints.back().first != sfz::Default::velocityRange.getEnd())
                region.velocityPoints.push_back(std::make_pair<int, float>(127, 1.0f));
            if (region.velocityPoints.front().first != sfz::Default::velocityRange.getStart())
                region.velocityPoints.insert(region.velocityPoints.begin(), std::make_pair<int, float>(0, 0.0f));
        } else {
            if (region.velocityPoints.front().first != sfz::Default::velocityRange.getEnd())
                region.velocityPoints.insert(region.velocityPoints.begin(), std::make_pair<int, float>(127, 0.0f));
            if (region.velocityPoints.back().first != sfz::Default::velocityRange.getStart())
                region.velocityPoints.push_back(std::make_pair<int, float>(0, 1.0f));
        }
    }
}

bool sfz::Synth::loadSfzFile(const fs::path& file)
{
    AtomicDisabler callbackDisabler { canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    clear();
    auto parserReturned = sfz::Parser::loadSfzFile(file);
    if (!parserReturned)
        return false;

    if (regions.empty())
        return false;

    resources.filePool.setRootDirectory(this->originalDirectory);
    resources.logger.setPrefix(file.filename().string());

    auto currentRegion = regions.begin();
    auto lastRegion = regions.rbegin();
    auto removeCurrentRegion = [&currentRegion, &lastRegion]() {
        if (currentRegion->get() == nullptr)
            return;

        DBG("Removing the region with sample " << currentRegion->get()->sample);
        std::iter_swap(currentRegion, lastRegion);
        ++lastRegion;
    };

    size_t maxFilters { 0 };
    size_t maxEQs { 0 };

    while (currentRegion < lastRegion.base()) {
        auto region = currentRegion->get();

        if (!region->isGenerator()) {
            if (!resources.filePool.checkSample(region->sample)) {
                removeCurrentRegion();
                continue;
            }

            const auto fileInformation = resources.filePool.getFileInformation(region->sample);
            if (!fileInformation) {
                removeCurrentRegion();
                continue;
            }

            region->sampleEnd = std::min(region->sampleEnd, fileInformation->end);
            if (region->loopRange.getEnd() == Default::loopRange.getEnd())
                region->loopRange.setEnd(region->sampleEnd);

            if (fileInformation->loopBegin != Default::loopRange.getStart() &&
                fileInformation->loopEnd != Default::loopRange.getEnd()) {
                if (region->loopRange.getStart() == Default::loopRange.getStart())
                    region->loopRange.setStart(fileInformation->loopBegin);

                if (region->loopRange.getEnd() == Default::loopRange.getEnd())
                    region->loopRange.setEnd(fileInformation->loopEnd);

                if (!region->loopMode)
                    region->loopMode = SfzLoopMode::loop_continuous;
            }

            if (fileInformation->numChannels == 2)
                region->isStereo = true;

            // TODO: adjust with LFO targets
            const auto maxOffset { region->offset + region->offsetRandom };
            if (!resources.filePool.preloadFile(region->sample, maxOffset))
                removeCurrentRegion();
        }

        for (auto note = 0; note < 128; note++) {
            if (region->keyRange.containsWithEnd(note) ||
                (region->hasKeyswitches() && region->keyswitchRange.containsWithEnd(note)))
                noteActivationLists[note].push_back(region);
        }

        for (auto cc = 0; cc < config::numCCs; cc++) {
            if (region->ccTriggers.contains(cc) || region->ccConditions.contains(cc))
                ccActivationLists[cc].push_back(region);
        }

        // Defaults
        for (int ccIndex = 0; ccIndex < config::numCCs; ccIndex++) {
            region->registerCC(ccIndex, resources.midiState.getCCValue(ccIndex));
        }

        if (defaultSwitch) {
            region->registerNoteOn(*defaultSwitch, 127, 1.0);
            region->registerNoteOff(*defaultSwitch, 0, 1.0);
        }

        // Set the default frequencies on equalizers if needed
        if (region->equalizers.size() > 0
            && region->equalizers[0].frequency == Default::eqFrequencyUnset)
        {
            region->equalizers[0].frequency = Default::eqFrequency1;
            if (region->equalizers.size() > 1
                && region->equalizers[1].frequency == Default::eqFrequencyUnset)
            {
                region->equalizers[1].frequency = Default::eqFrequency2;
                if (region->equalizers.size() > 2
                    && region->equalizers[2].frequency == Default::eqFrequencyUnset)
                {
                    region->equalizers[2].frequency = Default::eqFrequency3;
                }
            }
        }

        addEndpointsToVelocityCurve(*region);
        region->registerPitchWheel(0);
        region->registerAftertouch(0);
        region->registerTempo(2.0f);
        maxFilters = max(maxFilters, region->filters.size());
        maxEQs = max(maxEQs, region->equalizers.size());

        ++currentRegion;
    }
    const auto remainingRegions = std::distance(regions.begin(), lastRegion.base());
    DBG("Removing " << (regions.size() - remainingRegions) << " out of " << regions.size() << " regions");
    regions.resize(remainingRegions);
    modificationTime = checkModificationTime();

    for (auto& voice: voices) {
        voice->setMaxFiltersPerVoice(maxFilters);
        voice->setMaxEQsPerVoice(maxEQs);
    }

    return parserReturned;
}

sfz::Voice* sfz::Synth::findFreeVoice() noexcept
{
    auto freeVoice = absl::c_find_if(voices, [](const auto& voice) { return voice->isFree(); });
    if (freeVoice != voices.end())
        return freeVoice->get();

    // Find voices that can be stolen
    voiceViewArray.clear();
    for (auto& voice : voices)
        if (voice->canBeStolen())
            voiceViewArray.push_back(voice.get());
    absl::c_sort(voices, [](const auto& lhs, const auto& rhs) { return lhs->getSourcePosition() > rhs->getSourcePosition(); });

    for (auto* voice : voiceViewArray) {
        if (voice->getMeanSquaredAverage() < config::voiceStealingThreshold) {
            voice->reset();
            return voice;
        }
    }

    return {};
}

int sfz::Synth::getNumActiveVoices() const noexcept
{
    auto activeVoices { 0 };
    for (const auto& voice : voices) {
        if (!voice->isFree())
            activeVoices++;
    }
    return activeVoices;
}

void sfz::Synth::garbageCollect() noexcept
{

}

void sfz::Synth::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    ASSERT(samplesPerBlock < config::maxBlockSize);

    AtomicDisabler callbackDisabler { canEnterCallback };

    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    this->samplesPerBlock = samplesPerBlock;
    this->tempBuffer.resize(samplesPerBlock);
    for (auto& voice : voices)
        voice->setSamplesPerBlock(samplesPerBlock);
}

void sfz::Synth::setSampleRate(float sampleRate) noexcept
{
    AtomicDisabler callbackDisabler { canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    this->sampleRate = sampleRate;
    for (auto& voice : voices)
        voice->setSampleRate(sampleRate);

    resources.filterPool.setSampleRate(sampleRate);
    resources.eqPool.setSampleRate(sampleRate);
}

void sfz::Synth::renderBlock(AudioSpan<float> buffer) noexcept
{
    ScopedFTZ ftz;
    const auto callbackStartTime = std::chrono::high_resolution_clock::now();

    buffer.fill(0.0f);

    resources.filePool.cleanupPromises();

    if (freeWheeling)
        resources.filePool.waitForBackgroundLoading();

    AtomicGuard callbackGuard { inCallback };
    if (!canEnterCallback)
        return;

    auto tempSpan = AudioSpan<float>(tempBuffer).first(buffer.getNumFrames());
    int numActiveVoices { 0 };
    for (auto& voice : voices) {
        if (!voice->isFree()) {
            numActiveVoices++;
            voice->renderBlock(tempSpan);
            buffer.add(tempSpan);
        }
    }

    buffer.applyGain(db2mag(volume));

    const auto callbackDuration = std::chrono::high_resolution_clock::now() - callbackStartTime;
    resources.logger.logCallbackTime(callbackDuration, numActiveVoices, buffer.getNumFrames());
}

void sfz::Synth::noteOn(int delay, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);

    resources.midiState.noteOnEvent(noteNumber, velocity);

    AtomicGuard callbackGuard { inCallback };
    if (!canEnterCallback)
        return;

    noteOnDispatch(delay, noteNumber, velocity);
}

void sfz::Synth::noteOff(int delay, int noteNumber, uint8_t velocity [[maybe_unused]]) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);

    resources.midiState.noteOffEvent(noteNumber, velocity);

    AtomicGuard callbackGuard { inCallback };
    if (!canEnterCallback)
        return;

    // FIXME: Some keyboards (e.g. Casio PX5S) can send a real note-off velocity. In this case, do we have a
    // way in sfz to specify that a release trigger should NOT use the note-on velocity?
    // auto replacedVelocity = (velocity == 0 ? sfz::getNoteVelocity(noteNumber) : velocity);
    const auto replacedVelocity = resources.midiState.getNoteVelocity(noteNumber);

    for (auto& voice : voices)
        voice->registerNoteOff(delay, noteNumber, replacedVelocity);

    noteOffDispatch(delay, noteNumber, replacedVelocity);
}

void sfz::Synth::noteOffDispatch(int delay, int noteNumber, uint8_t velocity) noexcept
{
    const auto randValue = randNoteDistribution(Random::randomGenerator);
    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOff(noteNumber, velocity, randValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, noteNumber, velocity, Voice::TriggerType::NoteOff);
        }
    }
}

void sfz::Synth::noteOnDispatch(int delay, int noteNumber, uint8_t velocity) noexcept
{
    const auto randValue = randNoteDistribution(Random::randomGenerator);
    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOn(noteNumber, velocity, randValue)) {
            for (auto& voice : voices) {
                if (voice->checkOffGroup(delay, region->group))
                    noteOffDispatch(delay, voice->getTriggerNumber(), voice->getTriggerValue());
            }

            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, noteNumber, velocity, Voice::TriggerType::NoteOn);
        }
    }
}

void sfz::Synth::cc(int delay, int ccNumber, uint8_t ccValue) noexcept
{
    ASSERT(ccNumber < config::numCCs);
    ASSERT(ccNumber >= 0);

    AtomicGuard callbackGuard { inCallback };
    if (!canEnterCallback)
        return;

    if (ccNumber == config::resetCC) {
        resetAllControllers(delay);
        return;
    }

    resources.midiState.ccEvent(ccNumber, ccValue);
    for (auto& voice : voices)
        voice->registerCC(delay, ccNumber, ccValue);

    for (auto& region : ccActivationLists[ccNumber]) {
        if (region->registerCC(ccNumber, ccValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, ccNumber, ccValue, Voice::TriggerType::CC);
        }
    }
}

void sfz::Synth::pitchWheel(int delay, int pitch) noexcept
{
    ASSERT(pitch <= 8192);
    ASSERT(pitch >= -8192);

    resources.midiState.pitchBendEvent(pitch);

    for (auto& region: regions) {
        region->registerPitchWheel(pitch);
    }

    for (auto& voice: voices) {
        voice->registerPitchWheel(delay, pitch);
    }
}
void sfz::Synth::aftertouch(int /* delay */, uint8_t /* aftertouch */) noexcept
{
}
void sfz::Synth::tempo(int /* delay */, float /* secondsPerQuarter */) noexcept
{

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
    return numCurves;
}

const sfz::Region* sfz::Synth::getRegionView(int idx) const noexcept
{
    return (size_t)idx < regions.size() ? regions[idx].get() : nullptr;
}

const sfz::Voice* sfz::Synth::getVoiceView(int idx) const noexcept
{
    return (size_t)idx < voices.size() ? voices[idx].get() : nullptr;
}

const std::vector<std::string>& sfz::Synth::getUnknownOpcodes() const noexcept
{
    return unknownOpcodes;
}
size_t sfz::Synth::getNumPreloadedSamples() const noexcept
{
    return resources.filePool.getNumPreloadedSamples();
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
    resetVoices(numVoices);
}

void sfz::Synth::resetVoices(int numVoices)
{
    AtomicDisabler callbackDisabler{ canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    voices.clear();
    for (int i = 0; i < numVoices; ++i)
        voices.push_back(std::make_unique<Voice>(resources));

    for (auto& voice: voices) {
        voice->setSampleRate(this->sampleRate);
        voice->setSamplesPerBlock(this->samplesPerBlock);
    }

    voiceViewArray.reserve(numVoices);
    this->numVoices = numVoices;
}

void sfz::Synth::setOversamplingFactor(sfz::Oversampling factor) noexcept
{
    AtomicDisabler callbackDisabler{ canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    for (auto& voice: voices)
        voice->reset();

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
    AtomicDisabler callbackDisabler{ canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    resources.filePool.setPreloadSize(preloadSize);
}

uint32_t sfz::Synth::getPreloadSize() const noexcept
{
    return resources.filePool.getPreloadSize();
}

void sfz::Synth::enableFreeWheeling() noexcept
{
    if (!freeWheeling) {
        freeWheeling = true;
        DBG("Enabling freewheeling");
    }
}
void sfz::Synth::disableFreeWheeling() noexcept
{
    if (freeWheeling) {
        freeWheeling = false;
        DBG("Disabling freewheeling");
    }
}

void sfz::Synth::resetAllControllers(int delay) noexcept
{
    AtomicGuard callbackGuard { inCallback };
    if (!canEnterCallback)
        return;

    resources.midiState.resetAllControllers();
    for (auto& voice: voices) {
        voice->registerPitchWheel(delay, 0);
        for (int cc = 0; cc < config::numCCs; ++cc)
            voice->registerCC(delay, cc, 0);
    }

    for (auto& region: regions) {
        for (int cc = 0; cc < config::numCCs; ++cc)
            region->registerCC(cc, 0);
    }
}

fs::file_time_type sfz::Synth::checkModificationTime()
{
    auto returnedTime = modificationTime;
    for (auto file: getIncludedFiles()) {
        if (file.empty()) {
            return fs::file_time_type::min();
            }
        const auto fileTime = fs::last_write_time(file);
        if (returnedTime < fileTime)
            returnedTime = fileTime;
    }
    return returnedTime;
}

bool sfz::Synth::shouldReloadFile()
{
    return (checkModificationTime() > modificationTime);
}
