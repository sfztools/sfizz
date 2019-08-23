#pragma once
#include "Parser.h"
#include "Region.h"
#include "SfzHelpers.h"
#include "FilePool.h"
#include <vector>
#include <set>
#include <optional>
#include <random>
#include <string_view>

namespace sfz
{

class Synth: public Parser
{
public:
    Synth()
    {
        for (int i = 0; i < config::numVoices; ++i)
            voices.push_back(std::make_unique<Voice>());
    }
    bool loadSfzFile(const std::filesystem::path& file) final;
    int getNumRegions() const noexcept { return static_cast<int>(regions.size()); }
    int getNumGroups() const noexcept { return numGroups; }
    int getNumMasters() const noexcept { return numMasters; }
    int getNumCurves() const noexcept { return numCurves; }
    const Region* getRegionView(int idx) const noexcept { return (size_t)idx < regions.size() ? regions[idx].get() : nullptr; }
    auto getUnknownOpcodes() { return unknownOpcodes; }
    size_t getNumPreloadedSamples() { return filePool.getNumPreloadedSamples(); }

    void prepareToPlay(int samplesPerBlock, double sampleRate)
    {
        this->samplesPerBlock = samplesPerBlock;
        this->sampleRate = sampleRate;
        this->tempBuffer = StereoBuffer<float>(samplesPerBlock);
    }

    void renderBlock(StereoBuffer<float>& buffer)
    {
        for (auto& voice: voices)
        {
            voice->renderBlock(tempBuffer);
            buffer.add(tempBuffer);
        }
    }

    void noteOn(int delay, int channel, int noteNumber, uint8_t velocity)
    {
        auto randValue = getUniform();
        for (auto& region: regions)
        {
            if (region->registerNoteOn(channel, noteNumber, velocity, randValue))
            {
                auto voice = findFreeVoice();
                if (voice == nullptr)
                    continue;
                
                voice->startVoice(region.get(), channel, noteNumber, velocity, Voice::TriggerType::NoteOn);
            }
        }
    }

    void noteOff(int delay, int channel, int noteNumber, uint8_t velocity)
    {
        auto randValue = getUniform();
        for (auto& region: regions)
        {
            if (region->registerNoteOff(channel, noteNumber, velocity, randValue))
            {
                auto voice = findFreeVoice();
                if (voice == nullptr)
                    continue;
                
                voice->startVoice(region.get(), channel, noteNumber, velocity, Voice::TriggerType::NoteOff);
                voice->startVoice(region.get(), channel, noteNumber, velocity, Voice::TriggerType::NoteOff);
            }
        }
    }
    void cc(int delay, int channel, int ccNumber, uint8_t ccValue);
    void pitchWheel(int delay, int channel, int pitch);
    void aftertouch(int delay, int channel, uint8_t aftertouch);
    void tempo(int delay, float secondsPerQuarter);

protected:
    void callback(std::string_view header, std::vector<Opcode> members) final;
private:
    Voice* findFreeVoice()
    {
        auto freeVoice = absl::c_find_if(voices, [](const auto& voice) { return voice->isFree(); });
        if (freeVoice == voices.end())
        {
            DBG("Voices are overloaded, can't start a new note");
            return {};
        }
        return freeVoice->get();
    }
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
    using RegionPtrVector = std::vector<Region*>;
    std::vector<std::unique_ptr<Region>> regions;
    std::vector<std::unique_ptr<Voice>> voices;
    std::array<RegionPtrVector, 128> noteActivationLists;
    std::array<RegionPtrVector, 128> ccActivationLists;
    void buildRegion(const std::vector<Opcode>& regionOpcodes);

    StereoBuffer<float> tempBuffer { config::defaultSamplesPerBlock };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    float sampleRate { config::defaultSampleRate };
    std::random_device rd { };
    std::mt19937 randomGenerator { rd() };
    std::uniform_real_distribution<float> randomDistribution { 0, 1 };
    float getUniform()
    {
        return randomDistribution(randomGenerator);
    }
};

}