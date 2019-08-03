#pragma once
#include "Parser.h"
#include "Region.h"
#include "SfzHelpers.h"
#include "FilePool.h"
#include <vector>
#include <set>
#include <optional>
#include <string_view>

namespace sfz
{

class Synth: public Parser
{
public:
    bool loadSfzFile(const std::filesystem::path& file) final;
    int getNumRegions() const noexcept { return regions.size(); }
    int getNumGroups() const noexcept { return numGroups; }
    int getNumMasters() const noexcept { return numMasters; }
    int getNumCurves() const noexcept { return numCurves; }
    const Region* getRegionView(int idx) const noexcept { return (size_t)idx < regions.size() ? regions[idx].get() : nullptr; }
    auto getUnknownOpcodes() { return unknownOpcodes; }
protected:
    void callback(std::string_view header, std::vector<Opcode> members) final;
private:
    bool hasGlobal { false };
    bool hasControl { false };
    int numGroups { 0 };
    int numMasters { 0 };
    int numCurves { 0 };
    void clear();
    void handleGlobalOpcodes(const std::vector<Opcode>& members);
    void handleControlOpcodes(const std::vector<Opcode>& members);
    std::vector<Opcode> globalOpcodes;
    std::vector<Opcode> masterOpcodes;
    std::vector<Opcode> groupOpcodes;

    FilePool filePool;
    CCValueArray ccState;
    std::vector<CCNamePair> ccNames;
    std::optional<uint8_t> defaultSwitch;
    std::set<std::string_view> unknownOpcodes;
    using RegionPtrVector = std::vector<std::shared_ptr<Region>>;
    std::vector<std::shared_ptr<Region>> regions;
    std::array<RegionPtrVector, 128> noteActivationLists;
    std::array<RegionPtrVector, 128> ccActivationLists;
    void buildRegion(const std::vector<Opcode>& regionOpcodes);
};

}