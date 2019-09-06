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

#include "Voice.h"
#include "Defaults.h"
#include "SIMDHelpers.h"
#include "SfzHelpers.h"
#include "absl/algorithm/container.h"

sfz::Voice::Voice(const CCValueArray& ccState)
    : ccState(ccState)
{
}

void sfz::Voice::startVoice(Region* region, int delay, int channel, int number, uint8_t value, sfz::Voice::TriggerType triggerType) noexcept
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
    if (triggerType != TriggerType::CC)
        baseGain *= region->getNoteGain(number, value);
    baseGain *= region->getCCGain(ccState);
    if (region->amplitudeCC)
        baseGain *= normalizeCC(ccState[region->amplitudeCC->first]) * normalizePercents(region->amplitudeCC->second);
    amplitudeEnvelope.reset(baseGain);

    sourcePosition = region->getOffset();
    initialDelay = delay + region->getDelay();
    baseFrequency = midiNoteFrequency(number) * pitchRatio;
    prepareEGEnvelope(delay, value);
}

void sfz::Voice::prepareEGEnvelope(int delay, uint8_t velocity) noexcept
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

void sfz::Voice::setFileData(std::unique_ptr<AudioBuffer<float>> file) noexcept
{
    // DBG("File data set for sample " << region->sample);
    fileData = std::move(file);
    dataReady.store(true);
}

bool sfz::Voice::isFree() const noexcept
{
    return (region == nullptr);
}

void sfz::Voice::release(int delay) noexcept
{
    if (state == State::playing) {
        state = State::release;
        egEnvelope.startRelease(delay);
    }
}

void sfz::Voice::registerNoteOff(int delay, int channel, int noteNumber, uint8_t velocity [[maybe_unused]]) noexcept
{
    if (region == nullptr)
        return;

    if (state != State::playing)
        return;

    if (triggerChannel == channel && triggerNumber == noteNumber) {
        noteIsOff = true;

        if (region->loopMode == SfzLoopMode::one_shot)
            return;

        if (ccState[64] < 63) {
            release(delay);
        }
    }
}

void sfz::Voice::registerCC(int delay, int channel [[maybe_unused]], int ccNumber, uint8_t ccValue) noexcept
{
    if (ccNumber == 64 && noteIsOff && ccValue < 63)
        release(delay);

    if (region->amplitudeCC && ccNumber == region->amplitudeCC->first) {
        const float newGain { normalizeCC(ccValue) * normalizePercents(region->amplitudeCC->second) * baseGain };
        amplitudeEnvelope.registerEvent(delay, newGain);
    }
}

void sfz::Voice::registerPitchWheel(int delay [[maybe_unused]], int channel [[maybe_unused]], int pitch [[maybe_unused]]) noexcept
{
    // TODO
}

void sfz::Voice::registerAftertouch(int delay [[maybe_unused]], int channel [[maybe_unused]], uint8_t aftertouch [[maybe_unused]]) noexcept
{
    // TODO
}

void sfz::Voice::registerTempo(int delay [[maybe_unused]], float secondsPerQuarter [[maybe_unused]]) noexcept
{
    // TODO
}

void sfz::Voice::setSampleRate(float sampleRate) noexcept
{
    this->sampleRate = sampleRate;
}

void sfz::Voice::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    this->samplesPerBlock = samplesPerBlock;
    tempBuffer1.resize(samplesPerBlock);
    tempBuffer2.resize(samplesPerBlock);
    indexBuffer.resize(samplesPerBlock);
    tempSpan1 = absl::MakeSpan(tempBuffer1);
    tempSpan2 = absl::MakeSpan(tempBuffer2);
    indexSpan = absl::MakeSpan(indexBuffer);
}

void sfz::Voice::renderBlock(AudioSpan<float> buffer) noexcept
{
    ASSERT(static_cast<int>(buffer.getNumFrames()) <= samplesPerBlock);
    buffer.fill(0.0f);

    if (state == State::idle || region == nullptr)
        return;

    if (region->isGenerator())
        fillWithGenerator(buffer);
    else
        fillWithData(buffer);

    if (region->isStereo())
        processStereo(buffer);
    else
        processMono(buffer);
    
    if (!egEnvelope.isSmoothing())
        reset();
}

void sfz::Voice::processMono(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    auto leftBuffer = buffer.getSpan(0);
    auto rightBuffer = buffer.getSpan(1);
    
    auto envelopeSpan = tempSpan1.first(numSamples);
    amplitudeEnvelope.getBlock(envelopeSpan);
    ::applyGain<float>(envelopeSpan, leftBuffer);

    egEnvelope.getBlock(envelopeSpan);
    ::applyGain<float>(envelopeSpan, leftBuffer);

    ::copy<float>(leftBuffer, rightBuffer);
}
void sfz::Voice::processStereo(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    auto envelopeSpan = tempSpan1.first(numSamples);
   
    amplitudeEnvelope.getBlock(envelopeSpan);
    buffer.applyGain(envelopeSpan);
    
    egEnvelope.getBlock(envelopeSpan);
    buffer.applyGain(envelopeSpan);
}

void sfz::Voice::fillWithData(AudioSpan<float> buffer) noexcept
{
    auto source { [&]() {
        if (region->canUsePreloadedData() || !dataReady)
            return AudioSpan<const float>(*region->preloadedData);
        else
            return AudioSpan<const float>(*fileData);
    }() };

    auto indices = indexSpan.first(buffer.getNumFrames());
    auto jumps = tempSpan1.first(buffer.getNumFrames());
    auto leftCoeffs = tempSpan1.first(buffer.getNumFrames());
    auto rightCoeffs = tempSpan2.first(buffer.getNumFrames());

    ::fill<float>(jumps, pitchRatio * speedRatio);

    if (region->shouldLoop() && region->trueSampleEnd() <= source.getNumFrames()) {
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
            source.getNumFrames() - 1);
    }

    auto ind = indices.data();
    auto leftCoeff = leftCoeffs.data();
    auto rightCoeff = rightCoeffs.data();
    auto left = buffer.getChannel(0);
    if (source.getNumChannels() == 1) {
        while (ind < indices.end()) {
            *left = source.getChannel(0)[*ind] * (*leftCoeff) + source.getChannel(0)[*ind + 1] * (*rightCoeff);
            left++;
            ind++;
            leftCoeff++;
            rightCoeff++;
        }
    } else {
        auto right = buffer.getChannel(1);
        while (ind < indices.end()) {
            *left = source.getChannel(0)[*ind] * (*leftCoeff) + source.getChannel(0)[*ind + 1] * (*rightCoeff);
            *right = source.getChannel(1)[*ind] * (*leftCoeff) + source.getChannel(1)[*ind + 1] * (*rightCoeff);
            left++;
            right++;
            ind++;
            leftCoeff++;
            rightCoeff++;
        }
    }

    if (!region->shouldLoop() && (floatPosition + 1.01) > source.getNumFrames()) {
        DBG("Releasing " << region->sample);
        auto last = std::distance(indices.begin(), absl::c_find(indices, region->trueSampleEnd() - 1));
        release(last);
    }
}

void sfz::Voice::fillWithGenerator(AudioSpan<float> buffer) noexcept
{
    if (region->sample != "*sine")
        return;

    float step = baseFrequency * twoPi<float> / sampleRate;
    phase = ::linearRamp<float>(tempSpan1, phase, step);

    ::sin<float>(tempSpan1.first(buffer.getNumFrames()), buffer.getSpan(0));
    ::copy<float>(buffer.getSpan(0), buffer.getSpan(1));

    sourcePosition += buffer.getNumFrames();
}

bool sfz::Voice::checkOffGroup(int delay, uint32_t group) noexcept
{
    if (region != nullptr && triggerType == TriggerType::NoteOn && region->offBy && *region->offBy == group) {
        DBG("Off group of sample " << region->sample);
        release(delay);
        return true;
    }

    return false;
}

int sfz::Voice::getTriggerNumber() const noexcept
{
    return triggerNumber;
}

int sfz::Voice::getTriggerChannel() const noexcept
{
    return triggerNumber;
}

uint8_t sfz::Voice::getTriggerValue() const noexcept
{
    return triggerNumber;
}

sfz::Voice::TriggerType sfz::Voice::getTriggerType() const noexcept
{
    return triggerType;
}

void sfz::Voice::reset() noexcept
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

void sfz::Voice::garbageCollect() noexcept
{
    if (state == State::idle && region == nullptr)
        fileData.reset();
}
