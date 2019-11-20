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

#include "Synth.h"
#include "AtomicGuard.h"
#include "Config.h"
#include "Debug.h"
#include "MidiState.h"
#include "ScopedFTZ.h"
#include "StringViewHelpers.h"
#include "absl/algorithm/container.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>
using namespace std::literals;

sfz::Synth::Synth(int numVoices)
{
    resetVoices(numVoices);
}

void sfz::Synth::callback(absl::string_view header, const std::vector<Opcode>& members)
{
    switch (hash(header)) {
    case hash("global"):
        // We shouldn't have multiple global headers in file
        // But apparently some instruments do not really care :)
        // ASSERT(!hasGlobal);
        globalOpcodes = members;
        handleGlobalOpcodes(members);
        hasGlobal = true;
        break;
    case hash("control"):
        // We shouldn't have multiple control headers in file
        ASSERT(!hasControl)
        hasControl = true;
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
    auto lastRegion = std::make_unique<Region>(midiState);

    auto parseOpcodes = [&](const auto& opcodes) {
        for (auto& opcode : opcodes) {
            const auto unknown = absl::c_find_if(unknownOpcodes, [&](absl::string_view sv) { return sv.compare(opcode.opcode) == 0; });
            if (unknown != unknownOpcodes.end()) {
                continue;
            }

            if (!lastRegion->parseOpcode(opcode))
                unknownOpcodes.insert(opcode.opcode);
        }
    };

    parseOpcodes(globalOpcodes);
    parseOpcodes(masterOpcodes);
    parseOpcodes(groupOpcodes);
    parseOpcodes(regionOpcodes);

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
    filePool.clear();
    hasGlobal = false;
    hasControl = false;
    numGroups = 0;
    numMasters = 0;
    numCurves = 0;
    fileTicket = -1;
    defaultSwitch = absl::nullopt;
    for (auto& state : midiState.cc)
        state = 0;
    ccNames.clear();
    globalOpcodes.clear();
    masterOpcodes.clear();
    groupOpcodes.clear();
}

void sfz::Synth::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member : members) {
        switch (hash(member.opcode)) {
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
        switch (hash(member.opcode)) {
        case hash("Set_cc"):
            [[fallthrough]];
        case hash("set_cc"):
            if (member.parameter && Default::ccRange.containsWithEnd(*member.parameter))
                setValueFromOpcode(member, midiState.cc[*member.parameter], Default::ccRange);
            break;
        case hash("Label_cc"):
            [[fallthrough]];
        case hash("label_cc"):
            if (member.parameter && Default::ccRange.containsWithEnd(*member.parameter))
                ccNames.emplace_back(*member.parameter, member.value);
            break;
        case hash("Default_path"):
            [[fallthrough]];
        case hash("default_path"): {
            auto stringPath = std::string(member.value.begin(), member.value.end());
            auto newPath = fs::path(stringPath);
            if (fs::exists(newPath))
                rootDirectory = newPath;
            break;
        }
        default:
            // Unsupported control opcode
            ASSERTFALSE;
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

bool sfz::Synth::loadSfzFile(const fs::path& filename)
{
    AtomicDisabler callbackDisabler { canEnterCallback };
    while (inCallback) {
        std::this_thread::sleep_for(1ms);
    }

    clear();
    auto parserReturned = sfz::Parser::loadSfzFile(filename);
    if (!parserReturned)
        return false;

    if (regions.empty())
        return false;

    filePool.setRootDirectory(this->rootDirectory);

    auto lastRegion = regions.end() - 1;
    auto currentRegion = regions.begin();
    while (currentRegion <= lastRegion) {
        auto region = currentRegion->get();

        if (!region->isGenerator()) {
            auto fileInformation = filePool.getFileInformation(region->sample, region->offset + region->offsetRandom);
            if (!fileInformation) {
                DBG("Removing the region with sample " << region->sample);
                std::iter_swap(currentRegion, lastRegion);
                lastRegion--;
                continue;
            }
            region->sampleEnd = std::min(region->sampleEnd, fileInformation->end);
            region->loopRange.shrinkIfSmaller(fileInformation->loopBegin, fileInformation->loopEnd);
            region->preloadedData = fileInformation->preloadedData;
            region->sampleRate = fileInformation->sampleRate;
        }

        for (auto note = 0; note < 128; note++) {
            if (region->keyRange.containsWithEnd(note) || region->keyswitchRange.containsWithEnd(note))
                noteActivationLists[note].push_back(region);
        }

        for (auto cc = 0; cc < 128; cc++) {
            if (region->ccTriggers.contains(cc) || region->ccConditions.contains(cc))
                ccActivationLists[cc].push_back(region);
        }

        // Defaults
        for (int ccIndex = 1; ccIndex < 128; ccIndex++)
            region->registerCC(region->channelRange.getStart(), ccIndex, midiState.cc[ccIndex]);

        if (defaultSwitch) {
            region->registerNoteOn(region->channelRange.getStart(), *defaultSwitch, 127, 1.0);
            region->registerNoteOff(region->channelRange.getStart(), *defaultSwitch, 0, 1.0);
        }

        addEndpointsToVelocityCurve(*region);
        region->registerPitchWheel(region->channelRange.getStart(), 0);
        region->registerAftertouch(region->channelRange.getStart(), 0);
        region->registerTempo(2.0f);

        currentRegion++;
    }

    DBG("Removed " << regions.size() - std::distance(regions.begin(), lastRegion) - 1 << " out of " << regions.size() << " regions.");
    regions.resize(std::distance(regions.begin(), lastRegion) + 1);

    return parserReturned;
}

sfz::Voice* sfz::Synth::findFreeVoice() noexcept
{
    auto freeVoice = absl::c_find_if(voices, [](const auto& voice) { return voice->isFree(); });
    if (freeVoice != voices.end())
        return freeVoice->get();

    // Find voices that can be stolen
    DBG("No free voice, trying to steal");
    voiceViewArray.clear();
    for (auto& voice : voices)
        if (voice->canBeStolen())
            voiceViewArray.push_back(voice.get());
    absl::c_sort(voices, [](const auto& lhs, const auto& rhs) { return lhs->getSourcePosition() > rhs->getSourcePosition(); });

    for (auto* voice : voiceViewArray) {
        DBG("Average voice power: " << voice->getMeanSquaredAverage());
        if (voice->getMeanSquaredAverage() < config::voiceStealingThreshold) {
            DBG("Stealing voice...");
            voice->reset();
            return voice;
        }
    }

    DBG("Voices are overloaded, can't start a new note");
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
    for (auto& voice : voices) {
        voice->garbageCollect();
    }
}

void sfz::Synth::setSamplesPerBlock(int samplesPerBlock) noexcept
{
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
}

void sfz::Synth::renderBlock(AudioSpan<float> buffer) noexcept
{
    ScopedFTZ ftz;
    buffer.fill(0.0f);
    
    if (!canEnterCallback)
        return;

    AtomicGuard callbackGuard { inCallback };

    auto tempSpan = AudioSpan<float>(tempBuffer).first(buffer.getNumFrames());
    for (auto& voice : voices) {
        voice->renderBlock(tempSpan);
        buffer.add(tempSpan);
    }

    buffer.applyGain(db2mag(volume));
}

void sfz::Synth::noteOn(int delay, int channel, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    // DBG("Received note " << noteNumber << "/" << +velocity << " ON at time " << delay);

    midiState.noteOn(noteNumber, velocity);

    if (!canEnterCallback)
        return;

    AtomicGuard callbackGuard { inCallback };

    auto randValue = randNoteDistribution(Random::randomGenerator);

    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOn(channel, noteNumber, velocity, randValue)) {
            for (auto& voice : voices) {
                if (voice->checkOffGroup(delay, region->group))
                    noteOff(delay, voice->getTriggerChannel(), voice->getTriggerNumber(), 0);
            }

            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, channel, noteNumber, velocity, Voice::TriggerType::NoteOn);
            if (!region->isGenerator()) {
                voice->expectFileData(fileTicket);
                filePool.enqueueLoading(voice, &region->sample, region->trueSampleEnd(), fileTicket++);
            }
        }
    }
}

void sfz::Synth::noteOff(int delay, int channel, int noteNumber, uint8_t velocity [[maybe_unused]]) noexcept
{
    ASSERT(noteNumber < 128);
    ASSERT(noteNumber >= 0);
    // DBG("Received note " << noteNumber << "/" << +velocity << " OFF at time " << delay);

    if (!canEnterCallback)
        return;

    AtomicGuard callbackGuard { inCallback };

    // FIXME: Some keyboards (e.g. Casio PX5S) can send a real note-off velocity. In this case, do we have a
    // way in sfz to specify that a release trigger should NOT use the note-on velocity?
    // auto replacedVelocity = (velocity == 0 ? sfz::getNoteVelocity(noteNumber) : velocity);
    auto replacedVelocity = midiState.getNoteVelocity(noteNumber);
    auto randValue = randNoteDistribution(Random::randomGenerator);
    for (auto& voice : voices)
        voice->registerNoteOff(delay, channel, noteNumber, replacedVelocity);

    for (auto& region : noteActivationLists[noteNumber]) {
        if (region->registerNoteOff(channel, noteNumber, replacedVelocity, randValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, channel, noteNumber, replacedVelocity, Voice::TriggerType::NoteOff);
            if (!region->isGenerator()) {
                voice->expectFileData(fileTicket);
                filePool.enqueueLoading(voice, &region->sample, region->trueSampleEnd(), fileTicket++);
            }
        }
    }
}

void sfz::Synth::cc(int delay, int channel, int ccNumber, uint8_t ccValue) noexcept
{
    ASSERT(ccNumber < 128);
    ASSERT(ccNumber >= 0);

    if (!canEnterCallback)
        return;

    AtomicGuard callbackGuard { inCallback };

    for (auto& voice : voices)
        voice->registerCC(delay, channel, ccNumber, ccValue);

    midiState.cc[ccNumber] = ccValue;

    for (auto& region : ccActivationLists[ccNumber]) {
        if (region->registerCC(channel, ccNumber, ccValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region, delay, channel, ccNumber, ccValue, Voice::TriggerType::CC);
            if (!region->isGenerator()) {
                voice->expectFileData(fileTicket);
                filePool.enqueueLoading(voice, &region->sample, region->trueSampleEnd(), fileTicket++);
            }
        }
    }
}

void sfz::Synth::pitchWheel(int /* delay */, int /* channel */, int /* pitch */) noexcept
{

}
void sfz::Synth::aftertouch(int /* delay */, int /* channel */, uint8_t /* aftertouch */) noexcept
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
std::set<absl::string_view> sfz::Synth::getUnknownOpcodes() const noexcept
{
    return unknownOpcodes;
}
size_t sfz::Synth::getNumPreloadedSamples() const noexcept
{
    return filePool.getNumPreloadedSamples();
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
        voices.push_back(std::make_unique<Voice>(midiState));
    voiceViewArray.reserve(numVoices);
    this->numVoices = numVoices;
}