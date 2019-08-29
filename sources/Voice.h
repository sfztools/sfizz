#pragma once
#include "ADSREnvelope.h"
#include "Defaults.h"
#include "Globals.h"
#include "Region.h"
#include "SIMDHelpers.h"
#include "SfzHelpers.h"
#include "StereoBuffer.h"
#include "StereoSpan.h"
#include "absl/types/span.h"
#include <atomic>
#include <memory>

namespace sfz {
class Voice {
public:
    Voice() = delete;
    Voice(const CCValueArray& ccState)
        : ccState(ccState)
    {
    }
    enum class TriggerType {
        NoteOn,
        NoteOff,
        CC
    };
    void startVoice(Region* region, int delay, int channel, int number, uint8_t value, TriggerType triggerType)
    {
        this->triggerType = triggerType;
        triggerNumber = number;
        triggerChannel = channel;
        triggerValue = value;

        this->region = region;

        ASSERT(delay >= 0);
        if (delay < 0)
            delay = 0;

        DBG("Starting voice with " << region->sample);

        state = State::playing;
        speedRatio = static_cast<float>(region->sampleRate / this->sampleRate);
        pitchRatio = region->getBasePitchVariation(number, value);
        baseGain = region->getBaseGain();
        sourcePosition = region->getOffset();
        initialDelay = delay + region->getDelay();
        baseFrequency = midiNoteFrequency(number) * pitchRatio;
        prepareEGEnvelope(delay, value);
    }

    void prepareEGEnvelope(int delay, uint8_t velocity)
    {
        auto secondsToSamples = [this](auto timeInSeconds) {
            return static_cast<int>(timeInSeconds * sampleRate);
        };

        egEnvelope.reset(
            secondsToSamples(region->amplitudeEG.getAttack(ccState, velocity)),
            secondsToSamples(region->amplitudeEG.getRelease(ccState, velocity)),
            normalizePercents(region->amplitudeEG.getSustain(ccState, velocity)),
            delay + secondsToSamples(region->amplitudeEG.getDelay(ccState, velocity)),
            secondsToSamples(region->amplitudeEG.getDecay(ccState, velocity)),
            secondsToSamples(region->amplitudeEG.getHold(ccState, velocity)),
            normalizePercents(region->amplitudeEG.getStart(ccState, velocity)));
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

    void registerNoteOff(int delay, int channel, int noteNumber, uint8_t velocity [[maybe_unused]])
    {
        if (region == nullptr)
            return;

        if (state != State::playing)
            return;

        if (triggerChannel == channel && triggerNumber == noteNumber) {
            noteIsOff = true;

            if (region->loopMode == SfzLoopMode::one_shot)
                return;

            if (ccState[64] < 63)
                egEnvelope.startRelease(delay);
        }
    }

    void registerCC(int delay [[maybe_unused]], int channel [[maybe_unused]], int ccNumber [[maybe_unused]], uint8_t ccValue [[maybe_unused]])
    {
        if (ccNumber == 64 && noteIsOff && ccValue < 63)
            egEnvelope.startRelease(delay);
    }

    void registerPitchWheel(int delay, int channel, int pitch);
    void registerAftertouch(int delay, int channel, uint8_t aftertouch);
    void registerTempo(int delay, float secondsPerQuarter);

    void setSampleRate(float sampleRate)
    {
        this->sampleRate = sampleRate;
    }

    void setSamplesPerBlock(int samplesPerBlock)
    {
        this->samplesPerBlock = samplesPerBlock;
        tempBuffer1.resize(samplesPerBlock);
        tempBuffer2.resize(samplesPerBlock);
        indexBuffer.resize(samplesPerBlock);
        tempSpan1 = absl::MakeSpan(tempBuffer1);
        tempSpan2 = absl::MakeSpan(tempBuffer2);
        indexSpan = absl::MakeSpan(indexBuffer);
    }

    void renderBlock(StereoSpan<float> buffer)
    {
        const auto numSamples = buffer.size();
        ASSERT(static_cast<int>(numSamples) <= samplesPerBlock);
        buffer.fill(0.0f);

        if (state == State::idle || region == nullptr)
            return;

        if (region->isGenerator())
            fillWithGenerator(buffer);
        else 
            fillWithData(buffer);

        // buffer.applyGain(baseGain);

        auto envelopeSpan = tempSpan1.first(numSamples);
        egEnvelope.getBlock(envelopeSpan);
        // buffer.applyGain(envelopeSpan);

        if (!egEnvelope.isSmoothing())
            reset();
    }

    void fillWithData(StereoSpan<float> buffer)
    {
        auto source { [&]() {
            if (region->canUsePreloadedData() || !dataReady)
                return StereoSpan<const float>(*region->preloadedData);
            else
                return StereoSpan<const float>(*fileData);
        }()};

        auto indices = indexSpan.first(buffer.size());
        auto jumps = tempSpan1.first(buffer.size());
        auto leftCoeffs = tempSpan1.first(buffer.size());
        auto rightCoeffs = tempSpan2.first(buffer.size());

        ::fill<float>(jumps, pitchRatio * speedRatio);
        if (region->shouldLoop() && region->trueSampleEnd() <= source.size()) {
            floatPosition = ::loopingSFZIndex<float, false>(
                jumps,
                leftCoeffs,
                rightCoeffs,
                indices,
                floatPosition,
                region->trueSampleEnd() - 1,
                region->loopRange.getStart());
        } else {
            floatPosition = ::saturatingSFZIndex<float, false>(
                jumps,
                leftCoeffs,
                rightCoeffs,
                indices,
                floatPosition,
                source.size() - 1);
        }

        auto ind = indices.data();
        auto leftCoeff = leftCoeffs.data();
        auto rightCoeff = rightCoeffs.data();
        auto left = buffer.left().data();
        auto right = buffer.right().data();
        while (ind < indices.end()) {
            *left = source.left()[*ind] * (*leftCoeff) + source.left()[*ind + 1] * (*rightCoeff);
            *right = source.right()[*ind] * (*leftCoeff) + source.right()[*ind + 1] * (*rightCoeff);
            left++;
            right++;
            ind++;
            leftCoeff++;
            rightCoeff++;
        }

        if (!region->shouldLoop() && (floatPosition + 1.01) > source.size()) {
            DBG("Releasing " << region->sample);
            egEnvelope.startRelease(buffer.size());
        }
    }

    void fillWithGenerator(StereoSpan<float> buffer)
    {
        if (region->sample != "*sine")
            return;

        float step = baseFrequency * twoPi<float> / sampleRate;
        phase = ::linearRamp<float>(tempSpan1, phase, step);
        ::sin<float>(tempSpan1.first(buffer.size()), buffer.left());
        absl::c_copy(buffer.left(), buffer.right().begin());

        sourcePosition += buffer.size();
    }

    bool checkOffGroup(int delay [[maybe_unused]], uint32_t group) noexcept
    {
        if (region != nullptr && triggerType == TriggerType::NoteOn && region->offBy && *region->offBy == group) {
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
        state = State::idle;
        if (region != nullptr) {
            DBG("Reset voice with sample " << region->sample);
        }
        sourcePosition = 0;
        floatPosition = 0.0f;
        region = nullptr;
        noteIsOff = false;
    }

    void garbageCollect()
    {
        if (state == State::idle && region == nullptr)
            fileData.reset();
    }

private:
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
    // std::unique_ptr<StereoBuffer<float>, std::function<void(StereoBuffer<float>*)>> fileData { nullptr };
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