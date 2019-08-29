#pragma once
#include "FilePool.h"
#include "Parser.h"
#include "Region.h"
#include "SfzHelpers.h"
#include "Helpers.h"
#include "StereoSpan.h"
#include "absl/types/span.h"
#include <chrono>
#include <optional>
#include <random>
#include <set>
#include <string_view>
#include <thread>
#include <vector>
using namespace std::literals;

namespace sfz {

class Synth : public Parser {
public:
    Synth();

    bool loadSfzFile(const std::filesystem::path& file) final;
    int getNumRegions() const noexcept { return static_cast<int>(regions.size()); }
    int getNumGroups() const noexcept { return numGroups; }
    int getNumMasters() const noexcept { return numMasters; }
    int getNumCurves() const noexcept { return numCurves; }
    const Region* getRegionView(int idx) const noexcept { return (size_t)idx < regions.size() ? regions[idx].get() : nullptr; }
    auto getUnknownOpcodes() { return unknownOpcodes; }
    size_t getNumPreloadedSamples() { return filePool.getNumPreloadedSamples(); }

    void setSamplesPerBlock(int samplesPerBlock);
    void setSampleRate(float sampleRate);
    void renderBlock(StereoSpan<float> buffer);

    void noteOn(int delay, int channel, int noteNumber, uint8_t velocity);
    void noteOff(int delay, int channel, int noteNumber, uint8_t velocity);
    void cc(int delay, int channel, int ccNumber, uint8_t ccValue);
    void pitchWheel(int delay, int channel, int pitch);
    void aftertouch(int delay, int channel, uint8_t aftertouch);
    void tempo(int delay, float secondsPerQuarter);

    void getNumActiveVoices() const;
    void garbageCollect();
protected:
    void callback(std::string_view header, const std::vector<Opcode>& members) final;

private:
    bool hasGlobal { false };
    bool hasControl { false };
    int numGroups { 0 };
    int numMasters { 0 };
    int numCurves { 0 };
    void clear();
    void handleGlobalOpcodes(const std::vector<Opcode>& members);
    void handleControlOpcodes(const std::vector<Opcode>& members);
    void buildRegion(const std::vector<Opcode>& regionOpcodes);
    
    std::vector<Opcode> globalOpcodes;
    std::vector<Opcode> masterOpcodes;
    std::vector<Opcode> groupOpcodes;

    FilePool filePool;
    CCValueArray ccState;
    Voice* findFreeVoice();
    std::vector<CCNamePair> ccNames;
    std::optional<uint8_t> defaultSwitch;
    std::set<std::string_view> unknownOpcodes;
    using RegionPtrVector = std::vector<Region*>;
    std::vector<std::unique_ptr<Region>> regions;
    std::vector<std::unique_ptr<Voice>> voices;
    std::array<RegionPtrVector, 128> noteActivationLists;
    std::array<RegionPtrVector, 128> ccActivationLists;

    StereoBuffer<float> tempBuffer { config::defaultSamplesPerBlock };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    float sampleRate { config::defaultSampleRate };
    std::random_device rd {};
    std::mt19937 randomGenerator { rd() };
    std::uniform_real_distribution<float> randNoteDistribution { 0, 1 };

    LEAK_DETECTOR(Synth);
};

}