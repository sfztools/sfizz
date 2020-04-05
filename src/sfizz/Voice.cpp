// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Macros.h"
#include "Voice.h"
#include "AudioSpan.h"
#include "Config.h"
#include "Defaults.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "SfzHelpers.h"
#include "absl/algorithm/container.h"
#include <memory>

sfz::Voice::Voice(sfz::Resources& resources)
: resources(resources)
{
    filters.reserve(config::filtersPerVoice);
    equalizers.reserve(config::eqsPerVoice);

    waveOscillator.init(sampleRate);
}

void sfz::Voice::startVoice(Region* region, int delay, int number, float value, sfz::Voice::TriggerType triggerType) noexcept
{
    ASSERT(value >= 0.0f && value <= 1.0f);

    this->triggerType = triggerType;
    triggerNumber = number;
    triggerValue = value;

    this->region = region;
    state = State::playing;

    ASSERT(delay >= 0);
    if (delay < 0)
        delay = 0;

    if (region->isGenerator()) {
        const WavetableMulti* wave = nullptr;
        switch (hash(region->sample)) {
        default:
        case hash("*silence"):
            break;
        case hash("*sine"):
            wave = resources.wavePool.getWaveSin();
            break;
        case hash("*triangle"): // fallthrough
        case hash("*tri"):
            wave = resources.wavePool.getWaveTriangle();
            break;
        case hash("*square"):
            wave = resources.wavePool.getWaveSquare();
            break;
        case hash("*saw"):
            wave = resources.wavePool.getWaveSaw();
            break;
        }
        waveOscillator.setWavetable(wave);
        waveOscillator.setPhase(region->getPhase());
    } else if (region->oscillator) {
        const WavetableMulti* wave = resources.wavePool.getFileWave(region->sample);
        waveOscillator.setWavetable(wave);
        waveOscillator.setPhase(region->getPhase());
    } else {
        currentPromise = resources.filePool.getFilePromise(region->sample);
        if (currentPromise == nullptr) {
            reset();
            return;
        }
        speedRatio = static_cast<float>(currentPromise->sampleRate / this->sampleRate);
    }
    pitchRatio = region->getBasePitchVariation(number, value);
    baseVolumedB = region->getBaseVolumedB(number);
    baseGain = region->getBaseGain();
    if (triggerType != TriggerType::CC)
        baseGain *= region->getNoteGain(number, value);

    // Check that we can handle the number of filters; filters should be cleared here
    ASSERT((filters.capacity() - filters.size()) >= region->filters.size());
    ASSERT((equalizers.capacity() - equalizers.size()) >= region->equalizers.size());

    const unsigned numChannels = region->isStereo ? 2 : 1;
    for (auto& filter: region->filters) {
        auto newFilter = resources.filterPool.getFilter(filter, numChannels, number, value);
        if (newFilter)
            filters.push_back(newFilter);
    }

    for (auto& eq: region->equalizers) {
        auto newEQ = resources.eqPool.getEQ(eq, numChannels, value);
        if (newEQ)
            equalizers.push_back(newEQ);
    }

    sourcePosition = region->getOffset();
    triggerDelay = delay;
    initialDelay = delay + static_cast<int>(region->getDelay() * sampleRate);
    baseFrequency = midiNoteFrequency(number);
    bendStepFactor = centsFactor(region->bendStep);
    egAmplitude.reset(region->amplitudeEG, *region, resources.midiState, delay, value, sampleRate);
    egPitch.reset(region->pitchEG, *region, resources.midiState, delay, value, sampleRate);
    egFilter.reset(region->filterEG, *region, resources.midiState, delay, value, sampleRate);
}

bool sfz::Voice::isFree() const noexcept
{
    return (state == State::idle);
}

void sfz::Voice::release(int delay, bool fastRelease) noexcept
{
    if (state != State::playing)
        return;

    if (egAmplitude.getRemainingDelay() > std::max(0, delay - initialDelay))
        reset();
    else {
        for (ADSREnvelope<float>* eg : { &egAmplitude, &egPitch, &egFilter })
            eg->startRelease(delay, fastRelease);
    }
}

void sfz::Voice::registerNoteOff(int delay, int noteNumber, float velocity) noexcept
{
    ASSERT(velocity >= 0.0 && velocity <= 1.0);
    UNUSED(velocity);

    if (region == nullptr)
        return;

    if (state != State::playing)
        return;

    if (triggerNumber == noteNumber) {
        noteIsOff = true;

        if (region->loopMode == SfzLoopMode::one_shot)
            return;

        if (!region->checkSustain || resources.midiState.getCCValue(config::sustainCC) < config::halfCCThreshold)
            release(delay);
    }
}

void sfz::Voice::registerCC(int delay, int ccNumber, float ccValue) noexcept
{
    ASSERT(ccValue >= 0.0 && ccValue <= 1.0);
    if (region == nullptr)
        return;

    if (state ==  State::idle)
        return;

    if (ccNumber == config::allNotesOffCC || ccNumber == config::allSoundOffCC) {
        reset();
        return;
    }

    if (region->checkSustain && noteIsOff && ccNumber == config::sustainCC && ccValue < config::halfCCThreshold)
        release(delay);
}

void sfz::Voice::registerPitchWheel(int delay, float pitch) noexcept
{
    if (state == State::idle)
        return;
    UNUSED(delay);
    UNUSED(pitch);
}

void sfz::Voice::registerAftertouch(int delay, uint8_t aftertouch) noexcept
{
    // TODO
    UNUSED(delay);
    UNUSED(aftertouch);
}

void sfz::Voice::registerTempo(int delay, float secondsPerQuarter) noexcept
{
    // TODO
    UNUSED(delay);
    UNUSED(secondsPerQuarter);
}

void sfz::Voice::setSampleRate(float sampleRate) noexcept
{
    this->sampleRate = sampleRate;

    waveOscillator.init(sampleRate);
}

void sfz::Voice::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    this->samplesPerBlock = samplesPerBlock;
    this->minEnvelopeDelay = samplesPerBlock / 2;
}

void sfz::Voice::renderBlock(AudioSpan<float> buffer) noexcept
{
    ASSERT(static_cast<int>(buffer.getNumFrames()) <= samplesPerBlock);
    buffer.fill(0.0f);

    if (state == State::idle || region == nullptr) {
        powerHistory.push(0.0f);
        return;
    }

    const auto delay = min(static_cast<size_t>(initialDelay), buffer.getNumFrames());
    auto delayed_buffer = buffer.subspan(delay);
    initialDelay -= static_cast<int>(delay);

    { // Fill buffer with raw data
        ScopedTiming logger { dataDuration };
        if (region->isGenerator() || region->oscillator)
            fillWithGenerator(delayed_buffer);
        else
            fillWithData(delayed_buffer);
    }

    if (region->isStereo) {
        ampStageStereo(buffer);
        panStageStereo(buffer);
        filterStageStereo(buffer);
    } else {
        ampStageMono(buffer);
        filterStageMono(buffer);
        panStageMono(buffer);
    }

    if (!egAmplitude.isSmoothing())
        reset();

    powerHistory.push(buffer.meanSquared());
    this->triggerDelay = absl::nullopt;
}

void sfz::Voice::amplitudeEnvelope(absl::Span<float> modulationSpan) noexcept
{
    const auto numSamples = modulationSpan.size();
    const auto xfCurve = region->crossfadeCCCurve;

    auto tempSpan = resources.bufferPool.getBuffer(numSamples);
    if (!tempSpan)
        return;

    // AmpEG envelope
    egAmplitude.getBlock(modulationSpan);

    // Amplitude envelope
    applyGain<float>(baseGain, modulationSpan);
    for (const auto& mod : region->amplitudeCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&mod](float x) { return x * mod.value; });
        applyGain<float>(*tempSpan, modulationSpan);
    }

    // Crossfade envelopes
    for (const auto& mod : region->crossfadeCCInRange) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&](float x) { return crossfadeIn(mod.value, x, xfCurve); });
        applyGain<float>(*tempSpan, modulationSpan);
    }
    for (const auto& mod : region->crossfadeCCOutRange) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&](float x) { return crossfadeOut(mod.value, x, xfCurve); });
        applyGain<float>(*tempSpan, modulationSpan);
    }

    // Volume envelope
    applyGain<float>(db2mag(baseVolumedB), modulationSpan);
    for (const auto& mod : region->volumeCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        multiplicativeEnvelope(events, *tempSpan, [&](float x) { return db2mag(x * mod.value); });
        applyGain<float>(*tempSpan, modulationSpan);
    }
}

void sfz::Voice::ampStageMono(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { amplitudeDuration };

    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);

    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan)
        return;

    amplitudeEnvelope(*modulationSpan);
    applyGain<float>(*modulationSpan, leftBuffer);
}

void sfz::Voice::ampStageStereo(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { amplitudeDuration };

    const auto numSamples = buffer.getNumFrames();
    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan)
        return;

    amplitudeEnvelope(*modulationSpan);
    buffer.applyGain(*modulationSpan);
}

void sfz::Voice::panStageMono(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { panningDuration };

    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const auto rightBuffer = buffer.getSpan(1);

    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    auto tempSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan || !tempSpan)
        return;

    // Prepare for stereo output
    copy<float>(leftBuffer, rightBuffer);

    // Apply panning
    fill<float>(*modulationSpan, region->pan);
    for (const auto& mod : region->panCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&mod](float x) { return x * mod.value; });
        add<float>(*tempSpan, *modulationSpan);
    }
    pan<float>(*modulationSpan, leftBuffer, rightBuffer);
}

void sfz::Voice::panStageStereo(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { panningDuration };
    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const auto rightBuffer = buffer.getSpan(1);

    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    auto tempSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan || !tempSpan)
        return;

    // Apply panning
    // panningModulation(*modulationSpan);
    fill<float>(*modulationSpan, region->pan);
    for (const auto& mod : region->panCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&mod](float x) { return x * mod.value; });
        add<float>(*tempSpan, *modulationSpan);
    }
    pan<float>(*modulationSpan, leftBuffer, rightBuffer);

    // Apply the width/position process
    // widthModulation(*modulationSpan);
    fill<float>(*modulationSpan, region->width);
    for (const auto& mod : region->widthCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&mod](float x) { return x * mod.value; });
        add<float>(*tempSpan, *modulationSpan);
    }
    width<float>(*modulationSpan, leftBuffer, rightBuffer);

    // positionModulation(*modulationSpan);
    fill<float>(*modulationSpan, region->position);
    for (const auto& mod : region->positionCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        linearEnvelope(events, *tempSpan, [&mod](float x) { return x * mod.value; });
        add<float>(*tempSpan, *modulationSpan);
    }
    pan<float>(*modulationSpan, leftBuffer, rightBuffer);
}

void sfz::Voice::filterStageMono(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { filterDuration };
    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const float* inputChannel[1] { leftBuffer.data() };
    float* outputChannel[1] { leftBuffer.data() };
    for (auto& filter : filters) {
        filter->process(inputChannel, outputChannel, numSamples);
    }

    for (auto& eq : equalizers) {
        eq->process(inputChannel, outputChannel, numSamples);
    }
}

void sfz::Voice::filterStageStereo(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { filterDuration };
    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const auto rightBuffer = buffer.getSpan(1);

    const float* inputChannels[2] { leftBuffer.data(), rightBuffer.data() };
    float* outputChannels[2] { leftBuffer.data(), rightBuffer.data() };

    for (auto& filter : filters) {
        filter->process(inputChannels, outputChannels, numSamples);
    }

    for (auto& eq : equalizers) {
        eq->process(inputChannels, outputChannels, numSamples);
    }
}

void sfz::Voice::fillWithData(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    if (numSamples == 0)
        return;

    if (currentPromise == nullptr) {
        DBG("[Voice] Missing promise during fillWithData");
        return;
    }

    auto source = currentPromise->getData();

    auto jumps = resources.bufferPool.getBuffer(numSamples);
    auto bends = resources.bufferPool.getBuffer(numSamples);
    auto leftCoeffs = resources.bufferPool.getBuffer(numSamples);
    auto rightCoeffs = resources.bufferPool.getBuffer(numSamples);
    auto indices = resources.bufferPool.getIndexBuffer(numSamples);
    if (!jumps || !bends || !indices || !rightCoeffs || !leftCoeffs)
        return;

    fill<float>(*jumps, pitchRatio * speedRatio);

    const auto events = resources.midiState.getPitchEvents();
    const auto bendLambda = [this](float bend) {
        const auto bendInCents = bend > 0.0f ? bend * static_cast<float>(region->bendUp) : -bend * static_cast<float>(region->bendDown);
        return centsFactor(bendInCents);
    };

    if (region->bendStep > 1)
        multiplicativeEnvelope(events, *bends, bendLambda, bendStepFactor);
    else
        multiplicativeEnvelope(events, *bends, bendLambda);
    applyGain<float>(*bends, *jumps);

    for (const auto& mod : region->tuneCC) {
        const auto events = resources.midiState.getCCEvents(mod.cc);
        multiplicativeEnvelope(events, *bends, [&](float x) { return centsFactor(x * mod.value); });
        applyGain<float>(*bends, *jumps);
    }

    const float pitchEgDepth = region->pitchEgDepth + triggerValue * region->pitchEgVel2depth;
    if (pitchEgDepth != 0) {
        auto pitchEnvelope = resources.bufferPool.getBuffer(numSamples);
        if (pitchEnvelope) {
            egPitch.getBlock(*pitchEnvelope);
            applyGain<float>(pitchEgDepth, *pitchEnvelope);
            for (float& x : *pitchEnvelope)
                x = centsFactor(x);
            applyGain<float>(*pitchEnvelope, *jumps);
        }
    }

    jumps->front() += floatPositionOffset;
    cumsum<float>(*jumps, *jumps);
    sfzInterpolationCast<float>(*jumps, *indices, *leftCoeffs, *rightCoeffs);
    add<int>(sourcePosition, *indices);

    if (region->shouldLoop() && region->loopEnd(currentPromise->oversamplingFactor) <= source.getNumFrames()) {
        const auto loopEnd = static_cast<int>(region->loopEnd(currentPromise->oversamplingFactor));
        const auto offset = loopEnd - static_cast<int>(region->loopStart(currentPromise->oversamplingFactor)) + 1;
        for (auto* index = indices->begin(); index < indices->end(); ++index) {
            if (*index > loopEnd) {
                const auto remainingElements = static_cast<size_t>(std::distance(index, indices->end()));
                subtract<int>(offset, { index, remainingElements });
            }
        }
    } else {
        const auto sampleEnd = min(
            static_cast<int>(region->trueSampleEnd(currentPromise->oversamplingFactor)),
            static_cast<int>(source.getNumFrames())
        ) - 2;
        for (auto* index = indices->begin(); index < indices->end(); ++index) {
            if (*index >= sampleEnd) {
                release(static_cast<int>(std::distance(indices->begin(), index)));
                const auto remainingElements = static_cast<size_t>(std::distance(index, indices->end()));
                if (source.getNumFrames() - 1 < region->trueSampleEnd(currentPromise->oversamplingFactor)) {
                    DBG("[sfizz] Underflow: source available samples "
                        << source.getNumFrames() << "/"
                        << region->trueSampleEnd(currentPromise->oversamplingFactor)
                        << " for sample " << region->sample);
                }
                fill<int>(indices->last(remainingElements), sampleEnd);
                fill<float>(leftCoeffs->last(remainingElements), 0.0f);
                fill<float>(rightCoeffs->last(remainingElements), 1.0f);
                break;
            }
        }
    }

    auto ind = indices->data();
    auto leftCoeff = leftCoeffs->data();
    auto rightCoeff = rightCoeffs->data();
    auto leftSource = source.getConstSpan(0);
    auto left = buffer.getChannel(0);
    if (source.getNumChannels() == 1) {
        while (ind < indices->end()) {
            *left = linearInterpolation(leftSource[*ind], leftSource[*ind + 1], *leftCoeff, *rightCoeff);
            incrementAll(ind, left, leftCoeff, rightCoeff);
        }
    } else {
        auto right = buffer.getChannel(1);
        auto rightSource = source.getConstSpan(1);
        while (ind < indices->end()) {
            *left = linearInterpolation(leftSource[*ind], leftSource[*ind + 1], *leftCoeff, *rightCoeff);
            *right = linearInterpolation(rightSource[*ind], rightSource[*ind + 1], *leftCoeff, *rightCoeff);
            incrementAll(ind, left, right, leftCoeff, rightCoeff);
        }
    }

    sourcePosition = indices->back();
    floatPositionOffset = rightCoeffs->back();
}

void sfz::Voice::fillWithGenerator(AudioSpan<float> buffer) noexcept
{
    const auto leftSpan = buffer.getSpan(0);
    const auto rightSpan  = buffer.getSpan(1);

    if (region->sample == "*noise") {
        absl::c_generate(leftSpan, [&](){ return noiseDist(Random::randomGenerator); });
        absl::c_generate(rightSpan, [&](){ return noiseDist(Random::randomGenerator); });
    } else {
        const auto numSamples = buffer.getNumFrames();
        auto frequencies = resources.bufferPool.getBuffer(numSamples);
        auto bends = resources.bufferPool.getBuffer(numSamples);
        if (!frequencies || !bends)
            return;

        float keycenterFrequency = midiNoteFrequency(region->pitchKeycenter);
        fill<float>(*frequencies, pitchRatio * keycenterFrequency);

        const auto events = resources.midiState.getPitchEvents();
        const auto bendLambda = [this](float bend) {
            const auto bendInCents = bend > 0.0f ? bend * static_cast<float>(region->bendUp) : -bend * static_cast<float>(region->bendDown);
            return centsFactor(bendInCents);
        };
        if (region->bendStep > 1)
            multiplicativeEnvelope(events, *bends, bendLambda, bendStepFactor);
        else
            multiplicativeEnvelope(events, *bends, bendLambda);
        applyGain<float>(*bends, *frequencies);

        for (const auto& mod : region->tuneCC) {
            const auto events = resources.midiState.getCCEvents(mod.cc);
            multiplicativeEnvelope(events, *bends, [&](float x) { return centsFactor(x * mod.value); });
            applyGain<float>(*bends, *frequencies);
        }

        const float pitchEgDepth = region->pitchEgDepth + triggerValue * region->pitchEgVel2depth;
        if (pitchEgDepth != 0) {
            auto pitchEnvelope = resources.bufferPool.getBuffer(numSamples);
            if (pitchEnvelope) {
                egPitch.getBlock(*pitchEnvelope);
                applyGain<float>(pitchEgDepth, *pitchEnvelope);
                for (float& x : *pitchEnvelope)
                    x = centsFactor(x);
                applyGain<float>(*pitchEnvelope, *frequencies);
            }
        }

        waveOscillator.processModulated(frequencies->data(), leftSpan.data(), buffer.getNumFrames());
        copy<float>(leftSpan, rightSpan);
    }
}

bool sfz::Voice::checkOffGroup(int delay, uint32_t group) noexcept
{
    if (region == nullptr)
        return false;

    if (delay <= this->triggerDelay)
        return false;

    if (triggerType == TriggerType::NoteOn && region->offBy == group) {
        release(delay, region->offMode == SfzOffMode::fast);
        return true;
    }

    return false;
}

int sfz::Voice::getTriggerNumber() const noexcept
{
    return triggerNumber;
}

float sfz::Voice::getTriggerValue() const noexcept
{
    return triggerValue;
}

sfz::Voice::TriggerType sfz::Voice::getTriggerType() const noexcept
{
    return triggerType;
}

void sfz::Voice::reset() noexcept
{
    state = State::idle;
    region = nullptr;
    currentPromise.reset();
    sourcePosition = 0;
    floatPositionOffset = 0.0f;
    noteIsOff = false;
    filters.clear();
    equalizers.clear();
}

float sfz::Voice::getMeanSquaredAverage() const noexcept
{
    return powerHistory.getAverage();
}

bool sfz::Voice::canBeStolen() const noexcept
{
    return state == State::idle || egAmplitude.isReleased();
}

uint32_t sfz::Voice::getSourcePosition() const noexcept
{
    return sourcePosition;
}

void sfz::Voice::setMaxFiltersPerVoice(size_t numFilters)
{
    // There are filters in there, this call is unexpected
    ASSERT(filters.size() == 0);
    filters.reserve(numFilters);
}

void sfz::Voice::setMaxEQsPerVoice(size_t numFilters)
{
    // There are filters in there, this call is unexpected
    ASSERT(equalizers.size() == 0);
    equalizers.reserve(numFilters);
}
