#include "Synth.h"
#include "Helpers.h"
#include <iostream>
#include <utility>
#include <algorithm>

void sfz::Synth::callback(std::string_view header, const std::vector<Opcode>& members)
{
    switch (hash(header))
    {
    case hash("global"):
        // We shouldn't have multiple global headers in file
        ASSERT(!hasGlobal);
        globalOpcodes = members;
        hasGlobal = true;
        break;
    case hash("control"):
        // We shouldn't have multiple control headers in file
        ASSERT(!hasControl)
        hasControl = true;
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
        // TODO: implement curves
        break;
    default:
        std::cerr << "Unknown header: " << header << '\n';
    }
}

void sfz::Synth::buildRegion(const std::vector<Opcode>& regionOpcodes)
{
    auto lastRegion = std::make_unique<Region>();
    
    auto parseOpcodes = [&](const auto& opcodes) {
        for (auto& opcode: opcodes)
            if (!lastRegion->parseOpcode(opcode))
                unknownOpcodes.insert(opcode.opcode);
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
    for (auto& state: ccState)
        state = 0;
    ccNames.clear();
    globalOpcodes.clear();
    masterOpcodes.clear();
    groupOpcodes.clear();
    regions.clear();
}

void sfz::Synth::handleGlobalOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member: members)
    {
        if (member.opcode == "sw_default")
            setValueFromOpcode(member, defaultSwitch, Default::keyRange);
    }
}

void sfz::Synth::handleControlOpcodes(const std::vector<Opcode>& members)
{
    for (auto& member: members)
    {
        switch (hash(member.opcode))
        {
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
    while (currentRegion <= lastRegion)
    {
        auto region = currentRegion->get();
        
        if (region->isGenerator())
        {
            currentRegion++;
            continue;
        }

        auto fileInformation = filePool.getFileInformation(region->sample);
        if (!fileInformation)
        {
            DBG("Removing the region with sample " << region->sample);
            std::iter_swap(currentRegion, lastRegion);
            lastRegion--;
            continue;
        }

        region->sampleEnd = std::min(region->sampleEnd, fileInformation->end);
        region->loopRange.shrinkIfSmaller(fileInformation->loopBegin, fileInformation->loopEnd);
        region->preloadedData = fileInformation->preloadedData;
        region->sampleRate = fileInformation->sampleRate;

        for (auto note = region->keyRange.getStart(); note <= region->keyRange.getEnd(); note++)
            noteActivationLists[note].push_back(region);

        for (auto cc = region->keyRange.getStart(); cc <= region->keyRange.getEnd(); cc++)
            ccActivationLists[cc].push_back(region);
        
        // Defaults
        for (int ccIndex = 1; ccIndex < 128; ccIndex++)
            region->registerCC(region->channelRange.getStart(), ccIndex, ccState[ccIndex]);

        if (defaultSwitch)
        {
            region->registerNoteOn(region->channelRange.getStart(), *defaultSwitch, 127, 1.0);
            region->registerNoteOff(region->channelRange.getStart(), *defaultSwitch, 0, 1.0);
        }
        region->registerPitchWheel(region->channelRange.getStart(), 0);
        region->registerAftertouch(region->channelRange.getStart(), 0);
        region->registerTempo(2.0f);
        
        currentRegion++;
    }

    DBG("Removed " << regions.size() - std::distance(regions.begin(), lastRegion) - 1 << " out of " << regions.size() << " regions.");
    regions.resize(std::distance(regions.begin(), lastRegion) + 1);
    return parserReturned;
}
