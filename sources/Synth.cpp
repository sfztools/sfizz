#include "Synth.h"
#include "Helpers.h"
#include <iostream>

void sfz::Synth::callback(std::string_view header, std::vector<Opcode> members)
{
    switch (hash(header))
    {
    case hash("global"):
        // We shouldn't have multiple global headers in file
        ASSERT(!hasGlobal);
        globalOpcodes = std::move(members);
        hasGlobal = true;
        break;
    case hash("control"):
        // We shouldn't have multiple control headers in file
        ASSERT(!hasControl)
        hasControl = true;
        break;
    case hash("master"):
        masterOpcodes = std::move(members);
        numMasters++;
        break;
    case hash("group"): 
        groupOpcodes = std::move(members);
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
    auto& lastRegion = regions.emplace_back();
    
    auto parseOpcodes = [&](const auto& opcodes) {
        for (auto& opcode: opcodes)
            if (!lastRegion.parseOpcode(opcode))
                unknownOpcodes.insert(opcode.opcode);
    };

    parseOpcodes(globalOpcodes);
    parseOpcodes(masterOpcodes);
    parseOpcodes(groupOpcodes);
    parseOpcodes(regionOpcodes);
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
        default:
            // Unsupported control opcode
            ASSERTFALSE;
        }
    }
}

bool sfz::Synth::loadSfzFile(const std::filesystem::path& filename)
{
    clear();
    return sfz::Parser::loadSfzFile(filename);
}
