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
    Synth()
    {
        for (int i = 0; i < config::numVoices; ++i)
            voices.push_back(std::make_unique<Voice>(ccState));
    }

    ~Synth()
    {
        
    }

    bool loadSfzFile(const std::filesystem::path& file) final;
    int getNumRegions() const noexcept { return static_cast<int>(regions.size()); }
    int getNumGroups() const noexcept { return numGroups; }
    int getNumMasters() const noexcept { return numMasters; }
    int getNumCurves() const noexcept { return numCurves; }
    const Region* getRegionView(int idx) const noexcept { return (size_t)idx < regions.size() ? regions[idx].get() : nullptr; }
    auto getUnknownOpcodes() { return unknownOpcodes; }
    size_t getNumPreloadedSamples() { return filePool.getNumPreloadedSamples(); }

    void setSamplesPerBlock(int samplesPerBlock)
    {
        DBG("[Synth] Samples per block set to " << samplesPerBlock);
        this->samplesPerBlock = samplesPerBlock;
        this->tempBuffer.resize(samplesPerBlock);
        for (auto& voice : voices)
            voice->setSamplesPerBlock(samplesPerBlock);
    }

    void setSampleRate(float sampleRate)
    {
        DBG("[Synth] Sample rate set to " << sampleRate);
        this->sampleRate = sampleRate;
        for (auto& voice : voices)
            voice->setSampleRate(sampleRate);
    }

    void renderBlock(StereoSpan<float> buffer)
    {
        ScopedFTZ ftz;
        buffer.fill(0.0f);
        StereoSpan<float> tempSpan { tempBuffer, buffer.size() };
        for (auto& voice : voices) {
            voice->renderBlock(tempSpan);
            buffer.add(tempSpan);
            tempBuffer.fill(0.0f);
        }
    }

    void noteOn(int delay, int channel, int noteNumber, uint8_t velocity)
    {
        auto randValue = getUniform();

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

    void noteOff(int delay, int channel, int noteNumber, uint8_t velocity)
    {
        auto randValue = getUniform();
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

    void cc(int delay, int channel, int ccNumber, uint8_t ccValue)
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

    void pitchWheel(int delay, int channel, int pitch);
    void aftertouch(int delay, int channel, uint8_t aftertouch);
    void tempo(int delay, float secondsPerQuarter);
    void getNumActiveVoices() const
    {
        auto activeVoices { 0 };
        for (const auto& voice : voices) {
            if (!voice->isFree())
                activeVoices++;
        }
    }

    void garbageCollect()
    {
        for (auto& voice : voices) {
            voice->garbageCollect();
        }
    }
protected:
    void callback(std::string_view header, const std::vector<Opcode>& members) final;

private:
    Voice* findFreeVoice()
    {
        auto freeVoice = absl::c_find_if(voices, [](const auto& voice) { return voice->isFree(); });
        if (freeVoice == voices.end()) {
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
    std::random_device rd {};
    std::mt19937 randomGenerator { rd() };
    std::uniform_real_distribution<float> randomDistribution { 0, 1 };

    bool threadsShouldQuit { false };
    std::thread garbageCollectionThread;

    float getUniform()
    {
        return randomDistribution(randomGenerator);
    }

    LEAK_DETECTOR(Synth);
};

}