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

#pragma once
#include "ADSREnvelope.h"
#include "Config.h"
#include "Region.h"
#include "StereoBuffer.h"
#include "StereoSpan.h"
#include "LeakDetector.h"
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
    bool checkOffGroup(int delay, uint32_t group) noexcept;

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
    void release(int delay) noexcept;
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