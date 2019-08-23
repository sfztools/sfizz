#pragma once
#include "StereoBuffer.h"
#include "Globals.h"
#include "Region.h"
#include <atomic>
#include <memory>

namespace sfz
{
class Voice
{
public:
    enum class TriggerType { NoteOn, NoteOff, CC };
    void startVoice(Region* region, int channel, int number, uint8_t value, TriggerType triggerType)
    {   
        this->triggerType = triggerType;
        triggerNumber = number;
        triggerChannel = channel;
        triggerValue = value;

        this->region = region;
    }

    void setFileData(std::unique_ptr<StereoBuffer<float>> file)
    {
        fileData = std::move(file);
        dataReady.store(true);
    }

    bool isFree()
    {
        return (region == nullptr);
    }

    void registerNoteOff(int delay, int channel, int noteNumber, uint8_t velocity);
    bool registerCC(int delay, int channel, int ccNumber, uint8_t ccValue);
    void registerPitchWheel(int delay, int channel, int pitch);
    void registerAftertouch(int delay, int channel, uint8_t aftertouch);
    void registerTempo(int delay, float secondsPerQuarter);

    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void renderBlock(StereoBuffer<float>& buffer)
    {
        buffer.fill(0.0f);
    }

    bool checkOffGroup(int delay [[maybe_unused]], uint32_t group) noexcept
    {
        if (region != nullptr && triggerType == TriggerType::NoteOn && region->offBy && *region->offBy == group )
        {
            // TODO: release
            return true;
        }

        return false;
    }

    int getTriggerNumber() const
    {
        return triggerNumber;
    }

    int getTriggerChannel() const
    {
        return triggerNumber;
    }

    uint8_t getTriggerValue() const
    {
        return triggerNumber;
    }

    TriggerType getTriggerType() const
    {
        return triggerType;
    }


    void reset()
    {
        dataReady.store(false);
    }
private:
    Region* region;

    TriggerType triggerType;
    int triggerNumber;
    int triggerChannel;
    uint8_t triggerValue;

    std::atomic<bool> dataReady;
    std::unique_ptr<StereoBuffer<float>> fileData;

    int samplesPerBlock { config::defaultSamplesPerBlock };
    double sampleRate { config::defaultSampleRate };
};

}