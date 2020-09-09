// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Voice.h"
#include "Macros.h"
#include "Defaults.h"
#include "ModifierHelpers.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "Panning.h"
#include "SfzHelpers.h"
#include "LFO.h"
#include "modulations/ModId.h"
#include "modulations/ModKey.h"
#include "modulations/ModMatrix.h"
#include "Interpolators.h"
#include "absl/algorithm/container.h"

sfz::Voice::Voice(int voiceNumber, sfz::Resources& resources)
: id{voiceNumber}, stateListener(nullptr), resources(resources)
{
    filters.reserve(config::filtersPerVoice);
    equalizers.reserve(config::eqsPerVoice);

    for (WavetableOscillator& osc : waveOscillators)
        osc.init(sampleRate);

    gainSmoother.setSmoothing(config::gainSmoothing, sampleRate);
    xfadeSmoother.setSmoothing(config::xfadeSmoothing, sampleRate);
}

sfz::Voice::~Voice()
{
}

void sfz::Voice::startVoice(Region* region, int delay, const TriggerEvent& event) noexcept
{
    ASSERT(event.value >= 0.0f && event.value <= 1.0f);

    this->region = region;
    if (region->disabled())
        return;

    triggerEvent = event;
    if (triggerEvent.type == TriggerEventType::CC)
        triggerEvent.number = region->pitchKeycenter;

    switchState(State::playing);

    ASSERT(delay >= 0);
    if (delay < 0)
        delay = 0;

    if (region->isGenerator()) {
        const WavetableMulti* wave = nullptr;
        switch (hash(region->sampleId.filename())) {
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
        const float phase = region->getPhase();
        const int quality = region->oscillatorQuality.value_or(Default::oscillatorQuality);
        for (WavetableOscillator& osc : waveOscillators) {
            osc.setWavetable(wave);
            osc.setPhase(phase);
            osc.setQuality(quality);
        }
        setupOscillatorUnison();
    } else if (region->oscillator) {
        const WavetableMulti* wave = resources.wavePool.getFileWave(region->sampleId.filename());
        const float phase = region->getPhase();
        const int quality = region->oscillatorQuality.value_or(Default::oscillatorQuality);
        for (WavetableOscillator& osc : waveOscillators) {
            osc.setWavetable(wave);
            osc.setPhase(phase);
            osc.setQuality(quality);
        }
        setupOscillatorUnison();
    } else {
        currentPromise = resources.filePool.getFilePromise(region->sampleId);
        if (currentPromise == nullptr) {
            switchState(State::cleanMeUp);
            return;
        }
        speedRatio = static_cast<float>(currentPromise->sampleRate / this->sampleRate);
    }

    // do Scala retuning and reconvert the frequency into a 12TET key number
    const float numberRetuned = resources.tuning.getKeyFractional12TET(triggerEvent.number);

    pitchRatio = region->getBasePitchVariation(numberRetuned, triggerEvent.value);

    // apply stretch tuning if set
    if (resources.stretch)
        pitchRatio *= resources.stretch->getRatioForFractionalKey(numberRetuned);

    baseVolumedB = region->getBaseVolumedB(triggerEvent.number);
    baseGain = region->getBaseGain();
    if (triggerEvent.type != TriggerEventType::CC)
        baseGain *= region->getNoteGain(triggerEvent.number, triggerEvent.value);
    gainSmoother.reset();
    resetCrossfades();

    // Check that we can handle the number of filters; filters should be cleared here
    ASSERT((filters.capacity() - filters.size()) >= region->filters.size());
    ASSERT((equalizers.capacity() - equalizers.size()) >= region->equalizers.size());

    const unsigned numChannels = region->isStereo() ? 2 : 1;
    for (auto& filter: region->filters) {
        auto newFilter = resources.filterPool.getFilter(filter, numChannels, triggerEvent.number, triggerEvent.value);
        if (newFilter)
            filters.push_back(newFilter);
    }

    for (auto& eq: region->equalizers) {
        auto newEQ = resources.eqPool.getEQ(eq, numChannels, triggerEvent.value);
        if (newEQ)
            equalizers.push_back(newEQ);
    }

    sourcePosition = region->getOffset();
    triggerDelay = delay;
    initialDelay = delay + static_cast<int>(region->getDelay() * sampleRate);
    baseFrequency = resources.tuning.getFrequencyOfKey(triggerEvent.number);
    bendStepFactor = centsFactor(region->bendStep);
    bendSmoother.setSmoothing(region->bendSmooth, sampleRate);
    bendSmoother.reset(centsFactor(region->getBendInCents(resources.midiState.getPitchBend())));
    egEnvelope.reset(region->amplitudeEG, *region, resources.midiState, delay, triggerEvent.value, sampleRate);

    resources.modMatrix.initVoice(id, region->getId(), delay);
    saveModulationTargets(region);
}

int sfz::Voice::getCurrentSampleQuality() const noexcept
{
    return (region && region->sampleQuality) ?
        *region->sampleQuality : resources.synthConfig.currentSampleQuality();
}

bool sfz::Voice::isFree() const noexcept
{
    return (state == State::idle);
}

void sfz::Voice::release(int delay) noexcept
{
    if (state != State::playing)
        return;

    if (egEnvelope.getRemainingDelay() > delay) {
        switchState(State::cleanMeUp);
    } else {
        egEnvelope.startRelease(delay);
    }

    resources.modMatrix.releaseVoice(id, region->getId(), delay);
}

void sfz::Voice::off(int delay) noexcept
{
    if (region->offMode == SfzOffMode::fast) {
        egEnvelope.setReleaseTime( Default::offTime );
    } else if (region->offMode == SfzOffMode::time) {
        egEnvelope.setReleaseTime(region->offTime);
    }

    release(delay);
}

void sfz::Voice::registerNoteOff(int delay, int noteNumber, float velocity) noexcept
{
    ASSERT(velocity >= 0.0 && velocity <= 1.0);
    UNUSED(velocity);

    if (region == nullptr)
        return;

    if (state != State::playing)
        return;

    if (triggerEvent.number == noteNumber && triggerEvent.type == TriggerEventType::NoteOn) {
        noteIsOff = true;

        if (region->loopMode == SfzLoopMode::one_shot)
            return;

        if (!region->checkSustain || resources.midiState.getCCValue(region->sustainCC) < region->sustainThreshold)
            release(delay);
    }
}

void sfz::Voice::registerCC(int delay, int ccNumber, float ccValue) noexcept
{
    ASSERT(ccValue >= 0.0 && ccValue <= 1.0);
    if (region == nullptr)
        return;

    if (state != State::playing)
        return;

    if (region->checkSustain && noteIsOff && ccNumber == region->sustainCC && ccValue < region->sustainThreshold)
        release(delay);
}

void sfz::Voice::registerPitchWheel(int delay, float pitch) noexcept
{
    if (state != State::playing)
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
    gainSmoother.setSmoothing(config::gainSmoothing, sampleRate);
    xfadeSmoother.setSmoothing(config::xfadeSmoothing, sampleRate);

    for (WavetableOscillator& osc : waveOscillators)
        osc.init(sampleRate);

    for (auto& lfo : lfos)
        lfo->setSampleRate(sampleRate);

    powerFollower.setSampleRate(sampleRate);
}

void sfz::Voice::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    this->samplesPerBlock = samplesPerBlock;
    powerFollower.setSamplesPerBlock(samplesPerBlock);
}

void sfz::Voice::renderBlock(AudioSpan<float> buffer) noexcept
{
    ASSERT(static_cast<int>(buffer.getNumFrames()) <= samplesPerBlock);
    buffer.fill(0.0f);

    if (region == nullptr)
        return;

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

    if (region->isStereo()) {
        ampStageStereo(buffer);
        panStageStereo(buffer);
        filterStageStereo(buffer);
    } else {
        ampStageMono(buffer);
        filterStageMono(buffer);
        panStageMono(buffer);
    }

    if (!egEnvelope.isSmoothing())
        switchState(State::cleanMeUp);

    powerFollower.process(buffer);

    age += buffer.getNumFrames();
    if (triggerDelay) {
        // Should be OK but just in case;
        age = min(age - *triggerDelay, 0);
        triggerDelay = absl::nullopt;
    }

#if 0
    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
#endif
}

void sfz::Voice::resetCrossfades() noexcept
{
    float xfadeValue { 1.0f };
    const auto xfCurve = region->crossfadeCCCurve;

    for (const auto& mod : region->crossfadeCCInRange) {
        const auto value = resources.midiState.getCCValue(mod.cc);
        xfadeValue *= crossfadeIn(mod.data, value, xfCurve);
    }

    for (const auto& mod : region->crossfadeCCOutRange) {
        const auto value = resources.midiState.getCCValue(mod.cc);
        xfadeValue *= crossfadeOut(mod.data, value, xfCurve);
    }

    xfadeSmoother.reset(xfadeValue);
}

void sfz::Voice::applyCrossfades(absl::Span<float> modulationSpan) noexcept
{
    const auto numSamples = modulationSpan.size();
    const auto xfCurve = region->crossfadeCCCurve;

    auto tempSpan = resources.bufferPool.getBuffer(numSamples);
    auto xfadeSpan = resources.bufferPool.getBuffer(numSamples);

    if (!tempSpan || !xfadeSpan)
        return;

    fill<float>(*xfadeSpan, 1.0f);

    bool canShortcut = true;
    for (const auto& mod : region->crossfadeCCInRange) {
        const auto& events = resources.midiState.getCCEvents(mod.cc);
        canShortcut &= (events.size() == 1);
        linearEnvelope(events, *tempSpan, [&](float x) {
            return crossfadeIn(mod.data, x, xfCurve);
        });
        applyGain<float>(*tempSpan, *xfadeSpan);
    }

    for (const auto& mod : region->crossfadeCCOutRange) {
        const auto& events = resources.midiState.getCCEvents(mod.cc);
        canShortcut &= (events.size() == 1);
        linearEnvelope(events, *tempSpan, [&](float x) {
            return crossfadeOut(mod.data, x, xfCurve);
        });
        applyGain<float>(*tempSpan, *xfadeSpan);
    }

    xfadeSmoother.process(*xfadeSpan, *xfadeSpan, canShortcut);
    applyGain<float>(*xfadeSpan, modulationSpan);
}


void sfz::Voice::amplitudeEnvelope(absl::Span<float> modulationSpan) noexcept
{
    const auto numSamples = modulationSpan.size();

    ModMatrix& mm = resources.modMatrix;

    // AmpEG envelope
    egEnvelope.getBlock(modulationSpan);

    // Amplitude envelope
    applyGain1<float>(baseGain, modulationSpan);
    if (float* mod = mm.getModulation(amplitudeTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            modulationSpan[i] *= normalizePercents(mod[i]);
    }

    // Volume envelope
    applyGain1<float>(db2mag(baseVolumedB), modulationSpan);
    if (float* mod = mm.getModulation(volumeTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            modulationSpan[i] *= db2mag(mod[i]);
    }

    // Smooth the gain transitions
    gainSmoother.process(modulationSpan, modulationSpan);
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
    applyCrossfades(*modulationSpan);
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
    applyCrossfades(*modulationSpan);
    buffer.applyGain(*modulationSpan);
}

void sfz::Voice::panStageMono(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { panningDuration };

    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const auto rightBuffer = buffer.getSpan(1);

    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan)
        return;

    ModMatrix& mm = resources.modMatrix;

    // Prepare for stereo output
    copy<float>(leftBuffer, rightBuffer);

    // Apply panning
    fill(*modulationSpan, region->pan);
    if (float* mod = mm.getModulation(panTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            (*modulationSpan)[i] += normalizePercents(mod[i]);
    }
    pan(*modulationSpan, leftBuffer, rightBuffer);
}

void sfz::Voice::panStageStereo(AudioSpan<float> buffer) noexcept
{
    ScopedTiming logger { panningDuration };
    const auto numSamples = buffer.getNumFrames();
    const auto leftBuffer = buffer.getSpan(0);
    const auto rightBuffer = buffer.getSpan(1);

    auto modulationSpan = resources.bufferPool.getBuffer(numSamples);
    if (!modulationSpan)
        return;

    ModMatrix& mm = resources.modMatrix;

    // Apply panning
    fill(*modulationSpan, region->pan);
    if (float* mod = mm.getModulation(panTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            (*modulationSpan)[i] += normalizePercents(mod[i]);
    }
    pan(*modulationSpan, leftBuffer, rightBuffer);

    // Apply the width/position process
    fill(*modulationSpan, region->width);
    if (float* mod = mm.getModulation(widthTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            (*modulationSpan)[i] += normalizePercents(mod[i]);
    }
    width(*modulationSpan, leftBuffer, rightBuffer);

    fill(*modulationSpan, region->position);
    if (float* mod = mm.getModulation(positionTarget)) {
        for (size_t i = 0; i < numSamples; ++i)
            (*modulationSpan)[i] += normalizePercents(mod[i]);
    }
    pan(*modulationSpan, leftBuffer, rightBuffer);

    // add +3dB to compensate for the 2 pan stages (-3dB each stage)
    applyGain1(1.4125375446227544f, leftBuffer);
    applyGain1(1.4125375446227544f, rightBuffer);
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
    auto coeffs = resources.bufferPool.getBuffer(numSamples);
    auto indices = resources.bufferPool.getIndexBuffer(numSamples);
    if (!jumps || !indices || !coeffs)
        return;

    fill(*jumps, pitchRatio * speedRatio);
    pitchEnvelope(*jumps);

    jumps->front() += floatPositionOffset;
    cumsum<float>(*jumps, *jumps);
    sfzInterpolationCast<float>(*jumps, *indices, *coeffs);
    add1<int>(sourcePosition, *indices);

    if (region->shouldLoop() && region->loopEnd(currentPromise->oversamplingFactor) <= source.getNumFrames()) {
        const auto loopEnd = static_cast<int>(region->loopEnd(currentPromise->oversamplingFactor));
        const auto offset = loopEnd - static_cast<int>(region->loopStart(currentPromise->oversamplingFactor)) + 1;
        for (auto* index = indices->begin(); index < indices->end(); ++index) {
            if (*index > loopEnd) {
                const auto remainingElements = static_cast<size_t>(std::distance(index, indices->end()));
                subtract1<int>(offset, { index, remainingElements });
            }
        }
    } else {
        const auto sampleEnd = min(
            static_cast<int>(region->trueSampleEnd(currentPromise->oversamplingFactor)),
            static_cast<int>(source.getNumFrames())
        ) - 1;
        for (unsigned i = 0; i < indices->size(); ++i) {
            if ((*indices)[i] >= sampleEnd) {
#ifndef NDEBUG
                // Check for underflow
                if (source.getNumFrames() - 1 < region->trueSampleEnd(currentPromise->oversamplingFactor)) {
                    DBG("[sfizz] Underflow: source available samples "
                        << source.getNumFrames() << "/"
                        << region->trueSampleEnd(currentPromise->oversamplingFactor)
                        << " for sample " << region->sampleId);
                }
#endif
                egEnvelope.setReleaseTime(0.0f);
                egEnvelope.startRelease(i);
                fill<int>(indices->subspan(i), sampleEnd);
                fill<float>(coeffs->subspan(i), 1.0f);
                break;
            }
        }
    }

    const int quality = getCurrentSampleQuality();

    switch (quality) {
    default:
        if (quality > 2)
            goto high; // TODO sinc, not implemented
        // fall through
    case 1:
        fillInterpolated<kInterpolatorLinear>(source, buffer, *indices, *coeffs);
        break;
    case 2: high:
#if 1
        // B-spline response has faster decay of aliasing, but not zero-crossings at integer positions
        fillInterpolated<kInterpolatorBspline3>(source, buffer, *indices, *coeffs);
#else
        // Hermite polynomial
        fillInterpolated<kInterpolatorHermite3>(source, buffer, *indices, *coeffs);
#endif
        break;
    }

    sourcePosition = indices->back();
    floatPositionOffset = coeffs->back();

#if 0
    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
#endif
}

template <sfz::InterpolatorModel M>
void sfz::Voice::fillInterpolated(
    const sfz::AudioSpan<const float>& source, sfz::AudioSpan<float>& dest,
    absl::Span<const int> indices, absl::Span<const float> coeffs)
{
    auto ind = indices.data();
    auto coeff = coeffs.data();
    auto leftSource = source.getConstSpan(0);
    auto left = dest.getChannel(0);
    if (source.getNumChannels() == 1) {
        while (ind < indices.end()) {
            *left = sfz::interpolate<M>(&leftSource[*ind], *coeff);
            incrementAll(ind, left, coeff);
        }
    } else {
        auto right = dest.getChannel(1);
        auto rightSource = source.getConstSpan(1);
        while (ind < indices.end()) {
            *left = sfz::interpolate<M>(&leftSource[*ind], *coeff);
            *right = sfz::interpolate<M>(&rightSource[*ind], *coeff);
            incrementAll(ind, left, right, coeff);
        }
    }
}

void sfz::Voice::fillWithGenerator(AudioSpan<float> buffer) noexcept
{
    const auto leftSpan = buffer.getSpan(0);
    const auto rightSpan  = buffer.getSpan(1);

    if (region->sampleId.filename() == "*noise") {
        auto gen = [&]() {
            return uniformNoiseDist(Random::randomGenerator);
        };
        absl::c_generate(leftSpan, gen);
        absl::c_generate(rightSpan, gen);
    } else if (region->sampleId.filename() == "*gnoise") {
        // You need to wrap in a lambda, otherwise generate will
        // make a copy of the gaussian distribution *along with its state*
        // leading to periodic behavior....
        auto gen = [&]() {
            return gaussianNoiseDist();
        };
        absl::c_generate(leftSpan, gen);
        absl::c_generate(rightSpan, gen);
    } else {
        const auto numFrames = buffer.getNumFrames();

        auto frequencies = resources.bufferPool.getBuffer(numFrames);
        if (!frequencies)
            return;

        float keycenterFrequency = midiNoteFrequency(region->pitchKeycenter);
        fill(*frequencies, pitchRatio * keycenterFrequency);
        pitchEnvelope(*frequencies);

        if (waveUnisonSize == 1) {
            WavetableOscillator& osc = waveOscillators[0];
            osc.processModulated(frequencies->data(), 1.0, leftSpan.data(), buffer.getNumFrames());
            copy<float>(leftSpan, rightSpan);
        }
        else {
            buffer.fill(0.0f);

            auto tempSpan = resources.bufferPool.getBuffer(numFrames);
            if (!tempSpan)
                return;

            for (unsigned i = 0, n = waveUnisonSize; i < n; ++i) {
                WavetableOscillator& osc = waveOscillators[i];
                osc.processModulated(frequencies->data(), waveDetuneRatio[i], tempSpan->data(), numFrames);
                multiplyAdd1<float>(waveLeftGain[i], *tempSpan, leftSpan);
                multiplyAdd1<float>(waveRightGain[i], *tempSpan, rightSpan);
            }
        }
    }

#if 0
    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
#endif
}

bool sfz::Voice::checkOffGroup(const Region* other, int delay, int noteNumber) noexcept
{
    if (region == nullptr || other == nullptr)
        return false;

    if (triggerEvent.type == TriggerEventType::NoteOn
        && region->offBy == other->group
        && (region->group != other->group || noteNumber != triggerEvent.number)) {
        off(delay);
        return true;
    }

    return false;
}

void sfz::Voice::reset() noexcept
{
    switchState(State::idle);
    region = nullptr;
    currentPromise.reset();
    sourcePosition = 0;
    age = 0;
    floatPositionOffset = 0.0f;
    noteIsOff = false;

    powerFollower.clear();

    filters.clear();
    equalizers.clear();

    removeVoiceFromRing();
}

void sfz::Voice::setNextSisterVoice(Voice* voice) noexcept
{
    // Should never be null
    ASSERT(voice);
    nextSisterVoice = voice;
}

void sfz::Voice::setPreviousSisterVoice(Voice* voice) noexcept
{
    // Should never be null
    ASSERT(voice);
    previousSisterVoice = voice;
}

void sfz::Voice::removeVoiceFromRing() noexcept
{
    previousSisterVoice->setNextSisterVoice(nextSisterVoice);
    nextSisterVoice->setPreviousSisterVoice(previousSisterVoice);
    previousSisterVoice = this;
    nextSisterVoice = this;
}

float sfz::Voice::getAveragePower() const noexcept
{
    return powerFollower.getAveragePower();
}

bool sfz::Voice::releasedOrFree() const noexcept
{
    return state != State::playing || egEnvelope.isReleased();
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

void sfz::Voice::setMaxLFOsPerVoice(size_t numLFOs)
{
    lfos.resize(numLFOs);

    for (size_t i = 0; i < numLFOs; ++i) {
        auto lfo = absl::make_unique<LFO>();
        lfo->setSampleRate(sampleRate);
        lfos[i] = std::move(lfo);
    }
}

void sfz::Voice::setupOscillatorUnison()
{
    int m = region->oscillatorMulti;
    float d = region->oscillatorDetune;

    // 3-9: unison mode, 1: normal/RM, 2: PM/FM
    // TODO(jpc) RM/FM/PM synthesis
    if (m < 3) {
        waveUnisonSize = 1;
        waveDetuneRatio[0] = 1.0;
        waveLeftGain[0] = 1.0;
        waveRightGain[0] = 1.0;
        return;
    }

    // oscillator count, aka. unison size
    waveUnisonSize = m;

    // detune (cents)
    float detunes[config::oscillatorsPerVoice];
    detunes[0] = 0.0;
    detunes[1] = -d;
    detunes[2] = +d;
    for (int i = 3; i < m; ++i) {
        int n = (i - 1) / 2;
        detunes[i] = d * ((i & 1) ? -0.25f : +0.25f) * float(n);
    }

    // detune (ratio)
    for (int i = 0; i < m; ++i)
        waveDetuneRatio[i] = std::exp2(detunes[i] * (0.01f / 12.0f));

    // gains
    waveLeftGain[0] = 0.0;
    waveRightGain[m - 1] = 0.0;
    for (int i = 0; i < m - 1; ++i) {
        float g = 1.0f - float(i) / float(m - 1);
        waveLeftGain[m - 1 - i] = g;
        waveRightGain[i] = g;
    }

#if 0
    fprintf(stderr, "\n");
    fprintf(stderr, "# Left:\n");
    for (int i = m - 1; i >= 0; --i) {
        if (waveLeftGain[i] != 0)
            fprintf(stderr, "[%d] %10g cents, %10g dB\n", i, detunes[i], 20.0f * std::log10(waveLeftGain[i]));
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "# Right:\n");
    for (int i = 0; i < m; ++i) {
        if (waveRightGain[i] != 0)
            fprintf(stderr, "[%d] %10g cents, %10g dB\n", i, detunes[i], 20.0f * std::log10(waveRightGain[i]));
    }
#endif
}

void sfz::Voice::switchState(State s)
{
    if (s != state) {
        state = s;
        if (stateListener)
            stateListener->onVoiceStateChanged(id, s);
    }
}

void sfz::Voice::pitchEnvelope(absl::Span<float> pitchSpan) noexcept
{
    const auto numFrames = pitchSpan.size();
    auto bends = resources.bufferPool.getBuffer(numFrames);
    if (!bends)
        return;

    const auto events = resources.midiState.getPitchEvents();
    const auto bendLambda = [this](float bend) {
        return centsFactor(region->getBendInCents(bend));
    };

    if (region->bendStep > 1)
        pitchBendEnvelope(events, *bends, bendLambda, bendStepFactor);
    else
        pitchBendEnvelope(events, *bends, bendLambda);
    bendSmoother.process(*bends, *bends);
    applyGain<float>(*bends, pitchSpan);

    ModMatrix& mm = resources.modMatrix;

    if (float* mod = mm.getModulation(pitchTarget)) {
        for (size_t i = 0; i < numFrames; ++i)
            pitchSpan[i] *= centsFactor(mod[i]);
    }
}

void sfz::Voice::resetSmoothers() noexcept
{
    bendSmoother.reset(1.0f);
    gainSmoother.reset(0.0f);
}

void sfz::Voice::saveModulationTargets(const Region* region) noexcept
{
    ModMatrix& mm = resources.modMatrix;
    amplitudeTarget = mm.findTarget(ModKey::createNXYZ(ModId::Amplitude, region->getId()));
    volumeTarget = mm.findTarget(ModKey::createNXYZ(ModId::Volume, region->getId()));
    panTarget = mm.findTarget(ModKey::createNXYZ(ModId::Pan, region->getId()));
    positionTarget = mm.findTarget(ModKey::createNXYZ(ModId::Position, region->getId()));
    widthTarget = mm.findTarget(ModKey::createNXYZ(ModId::Width, region->getId()));
    pitchTarget = mm.findTarget(ModKey::createNXYZ(ModId::Pitch, region->getId()));
}
