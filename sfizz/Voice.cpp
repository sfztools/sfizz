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
#include "AudioSpan.h"
#include "Config.h"
#include "Defaults.h"
#include "MathHelpers.h"
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

    // DBG("Starting voice with " << region->sample);

    state = State::playing;
    speedRatio = static_cast<float>(region->sampleRate / this->sampleRate);
    pitchRatio = region->getBasePitchVariation(number, value);

    baseVolumedB = region->getBaseVolumedB();
    auto volumedB { baseVolumedB };
    if (region->volumeCC)
        volumedB += normalizeCC(ccState[region->volumeCC->first]) * region->volumeCC->second;
    volumeEnvelope.reset(db2mag(volumedB));
    // DBG("Base volume: " << baseVolumedB << " dB - with modifier: " << volumedB << " dB");

    baseGain = region->getBaseGain();
    baseGain *= region->getCrossfadeGain(ccState);
    if (triggerType != TriggerType::CC)
        baseGain *= region->getNoteGain(number, value);

    float gain { baseGain };
    if (region->amplitudeCC)
        gain *= normalizeCC(ccState[region->amplitudeCC->first]) * normalizePercents(region->amplitudeCC->second);
    amplitudeEnvelope.reset(gain);
    // DBG("Base gain: " << baseGain << " - with modifier: " << gain);

    basePan = normalizeNegativePercents(region->pan);
    auto pan = basePan;
    if (region->panCC)
        pan += normalizeCC(ccState[region->panCC->first]) * normalizeNegativePercents(region->panCC->second);
    panEnvelope.reset(pan);
    // DBG("Base pan: " << basePan << " - with modifier: " << pan);

    basePosition = normalizeNegativePercents(region->position);
    auto position = basePosition;
    if (region->positionCC)
        position += normalizeCC(ccState[region->positionCC->first]) * normalizeNegativePercents(region->positionCC->second);
    positionEnvelope.reset(position);
    // DBG("Base position: " << basePosition << " - with modifier: " << position);

    baseWidth = normalizeNegativePercents(region->width);
    auto width = baseWidth;
    if (region->widthCC)
        width += normalizeCC(ccState[region->widthCC->first]) * normalizeNegativePercents(region->widthCC->second);
    widthEnvelope.reset(width);
    // DBG("Base width: " << baseWidth << " - with modifier: " << width);

    sourcePosition = region->getOffset();
    floatPosition = static_cast<float>(sourcePosition);
    // DBG("Offset: " << floatPosition);
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

void sfz::Voice::setFileData(std::unique_ptr<AudioBuffer<float>> file, unsigned ticket) noexcept
{
    if (ticket != this->ticket)
        return;

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

        if (!region->checkSustain || ccState[config::sustainCC] < config::halfCCThreshold)
            release(delay);
    }
}

void sfz::Voice::registerCC(int delay, int channel [[maybe_unused]], int ccNumber, uint8_t ccValue) noexcept
{
    if (region == nullptr)
        return;

    if (ccNumber == config::sustainCC && noteIsOff && ccValue < config::halfCCThreshold)
        release(delay);

    if (region->amplitudeCC && ccNumber == region->amplitudeCC->first) {
        const float newGain { baseGain * normalizeCC(ccValue) * normalizePercents(region->amplitudeCC->second) };
        amplitudeEnvelope.registerEvent(delay, newGain);
    }

    if (region->volumeCC && ccNumber == region->volumeCC->first) {
        const float newVolumedB { baseVolumedB + normalizeCC(ccValue) * region->volumeCC->second };
        amplitudeEnvelope.registerEvent(delay, db2mag(newVolumedB));
    }

    if (region->panCC && ccNumber == region->panCC->first) {
        const float newPan { basePan + normalizeCC(ccValue) * normalizeNegativePercents(region->panCC->second)};
        panEnvelope.registerEvent(delay, newPan);
    }

    if (region->positionCC && ccNumber == region->positionCC->first) {
        const float newPosition { basePosition + normalizeCC(ccValue) * normalizeNegativePercents(region->positionCC->second)};
        positionEnvelope.registerEvent(delay, newPosition);
    }

    if (region->widthCC && ccNumber == region->widthCC->first) {
        const float newWidth { baseWidth + normalizeCC(ccValue) * normalizeNegativePercents(region->widthCC->second)};
        widthEnvelope.registerEvent(delay, newWidth);
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
    tempBuffer3.resize(samplesPerBlock);
    indexBuffer.resize(samplesPerBlock);
    tempSpan1 = absl::MakeSpan(tempBuffer1);
    tempSpan2 = absl::MakeSpan(tempBuffer2);
    tempSpan3 = absl::MakeSpan(tempBuffer3);
    indexSpan = absl::MakeSpan(indexBuffer);
}

void sfz::Voice::renderBlock(AudioSpan<float> buffer) noexcept
{
    ASSERT(static_cast<int>(buffer.getNumFrames()) <= samplesPerBlock);
    buffer.fill(0.0f);

    if (state == State::idle || region == nullptr) {
        powerHistory.push(0.0);
        return;
    }

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
    
    powerHistory.push(buffer.meanSquared());
}

void sfz::Voice::processMono(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    auto leftBuffer = buffer.getSpan(0);
    auto rightBuffer = buffer.getSpan(1);
    
    auto span1 = tempSpan1.first(numSamples);
    auto span2 = tempSpan2.first(numSamples);

    // Amplitude envelope
    amplitudeEnvelope.getBlock(span1);
    ::applyGain<float>(span1, leftBuffer);

    // AmpEG envelope
    egEnvelope.getBlock(span1);
    ::applyGain<float>(span1, leftBuffer);

    // Volume envelope
    volumeEnvelope.getBlock(span1);
    ::applyGain<float>(span1, leftBuffer);

    // Prepare for stereo output
    ::copy<float>(leftBuffer, rightBuffer);

    panEnvelope.getBlock(span1);
    // We assume that the pan envelope is already normalized between -1 and 1
    // Check bm_pan for your architecture to check if it's interesting to use the pan helper instead
    ::fill<float>(span2, 1.0f);
    ::add<float>(span1, span2);
    ::applyGain<float>(piFour<float>, span2);
    ::cos<float>(span2, span1);
    ::sin<float>(span2, span2);
    ::applyGain<float>(span1, leftBuffer);
    ::applyGain<float>(span2, rightBuffer);
}

void sfz::Voice::processStereo(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    auto span1 = tempSpan1.first(numSamples);
    auto span2 = tempSpan2.first(numSamples);
    auto span3 = tempSpan3.first(numSamples);
    auto leftBuffer = buffer.getSpan(0);
    auto rightBuffer = buffer.getSpan(1);

    // Amplitude envelope
    amplitudeEnvelope.getBlock(span1);
    buffer.applyGain(span1);
    
    // AmpEG envelope
    egEnvelope.getBlock(span1);
    buffer.applyGain(span1);

    // Volume envelope
    volumeEnvelope.getBlock(span1);
    buffer.applyGain(span1);

    // Create mid/side from left/right in the output buffer
    ::copy<float>(rightBuffer, span1);
    ::add<float>(leftBuffer, rightBuffer);
    ::subtract<float>(span1, leftBuffer);
    ::applyGain<float>(sqrtTwoInv<float>, leftBuffer);
    ::applyGain<float>(sqrtTwoInv<float>, rightBuffer);

    // Apply the width process
    widthEnvelope.getBlock(span1);
    ::fill<float>(span2, 1.0f);
    ::add<float>(span1, span2);
    ::applyGain<float>(piFour<float>, span2);
    ::cos<float>(span2, span1);
    ::sin<float>(span2, span2);
    ::applyGain<float>(span1, leftBuffer);
    ::applyGain<float>(span2, rightBuffer);

    // Apply a position to the "left" channel which is supposed to be our mid channel
    // TODO: add panning here too?
    positionEnvelope.getBlock(span1);
    ::fill<float>(span2, 1.0f);
    ::add<float>(span1, span2);
    ::applyGain<float>(piFour<float>, span2);
    ::cos<float>(span2, span1);
    ::sin<float>(span2, span2);
    ::copy<float>(leftBuffer, span3);
    ::copy<float>(rightBuffer, leftBuffer);
    ::multiplyAdd<float>(span1, span3, leftBuffer);
    ::multiplyAdd<float>(span2, span3, rightBuffer);
    ::applyGain<float>(sqrtTwoInv<float>, leftBuffer);
    ::applyGain<float>(sqrtTwoInv<float>, rightBuffer);
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
    return triggerChannel;
}

uint8_t sfz::Voice::getTriggerValue() const noexcept
{
    return triggerValue;
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

void sfz::Voice::expectFileData(unsigned ticket)
{
    this->ticket = ticket;
}

float sfz::Voice::getMeanSquaredAverage() const noexcept
{
    return powerHistory.getAverage();
}

bool sfz::Voice::canBeStolen() const noexcept
{
    return state == State::release;
}

float sfz::Voice::getSourcePosition() const noexcept
{
    return floatPosition;
}
