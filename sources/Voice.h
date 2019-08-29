#pragma once
#include "ADSREnvelope.h"
#include "Globals.h"
#include "Region.h"
#include "StereoBuffer.h"
#include "StereoSpan.h"
#include <absl/types/span.h>
#include <atomic>

namespace sfz {
class Voice {
public:
    Voice() = delete;
    Voice(const CCValueArray& ccState);
    enum class TriggerType {
        NoteOn,
        NoteOff,
        CC
    };
    void setSampleRate(float sampleRate) noexcept;
    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    
    void startVoice(Region* region, int delay, int channel, int number, uint8_t value, TriggerType triggerType) noexcept;

    void setFileData(std::unique_ptr<StereoBuffer<float>> file) noexcept;
    void registerNoteOff(int delay, int channel, int noteNumber, uint8_t velocity) noexcept;
    void registerCC(int delay, int channel, int ccNumber, uint8_t ccValue) noexcept;
    void registerPitchWheel(int delay, int channel, int pitch) noexcept;
    void registerAftertouch(int delay, int channel, uint8_t aftertouch) noexcept;
    void registerTempo(int delay, float secondsPerQuarter) noexcept;
    bool checkOffGroup(int delay [[maybe_unused]], uint32_t group) noexcept;

    void renderBlock(StereoSpan<float> buffer) noexcept;

    bool isFree() const noexcept;
    int getTriggerNumber() const noexcept;
    int getTriggerChannel() const noexcept;
    uint8_t getTriggerValue() const noexcept;
    TriggerType getTriggerType() const noexcept;

    void reset() noexcept;
    void garbageCollect() noexcept;
private:
    void fillWithData(StereoSpan<float> buffer) noexcept;
    void fillWithGenerator(StereoSpan<float> buffer) noexcept;
    void prepareEGEnvelope(int delay, uint8_t velocity) noexcept;

    Region* region { nullptr };

    enum class State {
        idle,
        playing,
        release
    };
    State state { State::idle };
    bool noteIsOff { false };

    TriggerType triggerType;
    int triggerNumber;
    int triggerChannel;
    uint8_t triggerValue;

    float speedRatio { 1.0 };
    float pitchRatio { 1.0 };
    float baseGain { 1.0 };
    float baseFrequency { 440.0 };
    float phase { 0.0f };

    uint32_t sourcePosition { 0 };
    float floatPosition { 0.0f };
    uint32_t initialDelay { 0 };

    std::atomic<bool> dataReady { false };
    std::unique_ptr<StereoBuffer<float>> fileData { nullptr };

    Buffer<float> tempBuffer1;
    Buffer<float> tempBuffer2;
    Buffer<int> indexBuffer;
    absl::Span<float> tempSpan1 { absl::MakeSpan(tempBuffer1) };
    absl::Span<float> tempSpan2 { absl::MakeSpan(tempBuffer2) };
    absl::Span<int> indexSpan { absl::MakeSpan(indexBuffer) };

    int samplesPerBlock { config::defaultSamplesPerBlock };
    double sampleRate { config::defaultSampleRate };

    const CCValueArray& ccState;
    ADSREnvelope<float> egEnvelope;
    LEAK_DETECTOR(Voice);
};

} // namespace sfz