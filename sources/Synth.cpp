#include "Synth.h"
#include "Helpers.h"
#include "absl/algorithm/container.h"
#include <algorithm>
#include <iostream>
#include <utility>

sfz::Synth::Synth()
{
    for (int i = 0; i < config::numVoices; ++i)
        voices.push_back(std::make_unique<Voice>(ccState));
}

void sfz::Synth::callback(std::string_view header, const std::vector<Opcode>& members)
{
    switch (hash(header)) {
    case hash("global"):
        // We shouldn't have multiple global headers in file
        ASSERT(!hasGlobal);
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
    auto lastRegion = std::make_unique<Region>();

    auto parseOpcodes = [&](const auto& opcodes) {
        for (auto& opcode : opcodes) {
            const auto unknown = absl::c_find_if(unknownOpcodes, [&](std::string_view sv) { return sv.compare(opcode.opcode) == 0; });
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
    hasGlobal = false;
    hasControl = false;
    numGroups = 0;
    numMasters = 0;
    numCurves = 0;
    defaultSwitch = std::nullopt;
    for (auto& state : ccState)
        state = 0;
    ccNames.clear();
    globalOpcodes.clear();
    masterOpcodes.clear();
    groupOpcodes.clear();
    regions.clear();
}

void sfz::Synth::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member : members) {
        switch (hash(member.opcode)) {
        case hash("sw_default"):
            setValueFromOpcode(member, defaultSwitch, Default::keyRange);
            break;
        }
    }
}

void sfz::Synth::handleControlOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member : members) {
        switch (hash(member.opcode)) {
        case hash("set_cc"):
            if (member.parameter && Default::ccRange.containsWithEnd(*member.parameter))
                setValueFromOpcode(member, ccState[*member.parameter], Default::ccRange);
            break;
        case hash("label_cc"):
            if (member.parameter && Default::ccRange.containsWithEnd(*member.parameter))
                ccNames.emplace_back(*member.parameter, member.value);
            break;
        case hash("default_path"):
            if (auto newPath = std::filesystem::path(member.value); std::filesystem::exists(newPath))
                rootDirectory = newPath;
            break;
        default:
            // Unsupported control opcode
            ASSERTFALSE;
        }
    }
}

void addEndpointsToVelocityCurve(sfz::Region& region)
{
    if (region.velocityPoints.size() > 0)
    {
        absl::c_sort(region.velocityPoints, [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
        if (region.ampVeltrack > 0)
        {
            if (region.velocityPoints.back().first != sfz::Default::velocityRange.getEnd())
                region.velocityPoints.push_back(std::make_pair<int, float>(127, 1.0f));
            if (region.velocityPoints.front().first != sfz::Default::velocityRange.getStart())
                region.velocityPoints.insert(region.velocityPoints.begin(), std::make_pair<int, float>(0, 0.0f));
        }
        else
        {
            if (region.velocityPoints.front().first != sfz::Default::velocityRange.getEnd())
                region.velocityPoints.insert(region.velocityPoints.begin(), std::make_pair<int, float>(127, 0.0f));
            if (region.velocityPoints.back().first != sfz::Default::velocityRange.getStart())
                region.velocityPoints.push_back(std::make_pair<int, float>(0, 1.0f));
        }        
    }
}

bool sfz::Synth::loadSfzFile(const std::filesystem::path& filename)
{
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
            auto fileInformation = filePool.getFileInformation(region->sample);
            if (!fileInformation) {
                DBG("Removing the region with sample " << region->sample);
                std::iter_swap(currentRegion, lastRegion);
                lastRegion--;
                continue;
            }

            region->numChannels = fileInformation->numChannels;
            region->sampleEnd = std::min(region->sampleEnd, fileInformation->end);
            region->loopRange.shrinkIfSmaller(fileInformation->loopBegin, fileInformation->loopEnd);
            region->preloadedData = fileInformation->preloadedData;
            region->sampleRate = fileInformation->sampleRate;
        }

        for (auto note = region->keyRange.getStart(); note <= region->keyRange.getEnd(); note++)
            noteActivationLists[note].push_back(region);

        for (auto cc = region->keyRange.getStart(); cc <= region->keyRange.getEnd(); cc++)
            ccActivationLists[cc].push_back(region);

        // Defaults
        for (int ccIndex = 1; ccIndex < 128; ccIndex++)
            region->registerCC(region->channelRange.getStart(), ccIndex, ccState[ccIndex]);

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
    if (freeVoice == voices.end()) {
        DBG("Voices are overloaded, can't start a new note");
        return {};
    }
    return freeVoice->get();
}

void sfz::Synth::getNumActiveVoices() const noexcept
{
    auto activeVoices { 0 };
    for (const auto& voice : voices) {
        if (!voice->isFree())
            activeVoices++;
    }
}

void sfz::Synth::garbageCollect() noexcept
{
    for (auto& voice : voices) {
        voice->garbageCollect();
    }
}

void sfz::Synth::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    DBG("[Synth] Samples per block set to " << samplesPerBlock);
    this->samplesPerBlock = samplesPerBlock;
    this->tempBuffer.resize(samplesPerBlock);
    for (auto& voice : voices)
        voice->setSamplesPerBlock(samplesPerBlock);
}

void sfz::Synth::setSampleRate(float sampleRate) noexcept
{
    DBG("[Synth] Sample rate set to " << sampleRate);
    this->sampleRate = sampleRate;
    for (auto& voice : voices)
        voice->setSampleRate(sampleRate);
}

void sfz::Synth::renderBlock(StereoSpan<float> buffer) noexcept
{
    ScopedFTZ ftz;
    buffer.fill(0.0f);
    StereoSpan<float> tempSpan { tempBuffer, buffer.size() };
    for (auto& voice : voices) {
        voice->renderBlock(tempSpan);
        buffer.add(tempSpan);
    }
}

void sfz::Synth::noteOn(int delay, int channel, int noteNumber, uint8_t velocity) noexcept
{
    auto randValue = randNoteDistribution(Random::randomGenerator);

    for (auto& region : regions) {
        if (region->registerNoteOn(channel, noteNumber, velocity, randValue)) {
            for (auto& voice : voices) {
                if (voice->checkOffGroup(delay, region->group))
                    noteOff(delay, voice->getTriggerChannel(), voice->getTriggerNumber(), 0);
            }

            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region.get(), delay, channel, noteNumber, velocity, Voice::TriggerType::NoteOn);
            filePool.enqueueLoading(voice, region->sample, region->trueSampleEnd());
        }
    }
}

void sfz::Synth::noteOff(int delay, int channel, int noteNumber, uint8_t velocity) noexcept
{
    auto randValue = randNoteDistribution(Random::randomGenerator);
    for (auto& voice : voices)
        voice->registerNoteOff(delay, channel, noteNumber, velocity);

    for (auto& region : regions) {
        if (region->registerNoteOff(channel, noteNumber, velocity, randValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region.get(), delay, channel, noteNumber, velocity, Voice::TriggerType::NoteOff);
            filePool.enqueueLoading(voice, region->sample, region->trueSampleEnd());
        }
    }
}

void sfz::Synth::cc(int delay, int channel, int ccNumber, uint8_t ccValue) noexcept
{
    for (auto& voice : voices)
        voice->registerCC(delay, channel, ccNumber, ccValue);

    ccState[ccNumber] = ccValue;

    for (auto& region : regions) {
        if (region->registerCC(channel, ccNumber, ccValue)) {
            auto voice = findFreeVoice();
            if (voice == nullptr)
                continue;

            voice->startVoice(region.get(), delay, channel, ccNumber, ccValue, Voice::TriggerType::CC);
            filePool.enqueueLoading(voice, region->sample, region->trueSampleEnd());
        }
    }
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
std::set<std::string_view> sfz::Synth::getUnknownOpcodes() const noexcept
{
    return unknownOpcodes;
}
size_t sfz::Synth::getNumPreloadedSamples() const noexcept
{
    return filePool.getNumPreloadedSamples();
}