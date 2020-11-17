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
#include "FlexEnvelope.h"
#include "modulations/ModId.h"
#include "modulations/ModKey.h"
#include "modulations/ModMatrix.h"
#include "Interpolators.h"
#include "absl/algorithm/container.h"

sfz::Voice::Voice(int voiceNumber, sfz::Resources& resources)
: id{voiceNumber}, stateListener(nullptr), resources(resources)
{
    for (unsigned i = 0; i < config::filtersPerVoice; ++i)
        filters.emplace_back(resources);

    for (unsigned i = 0; i < config::eqsPerVoice; ++i)
        equalizers.emplace_back(resources);

    for (WavetableOscillator& osc : waveOscillators)
        osc.init(sampleRate);

    gainSmoother.setSmoothing(config::gainSmoothing, sampleRate);
    xfadeSmoother.setSmoothing(config::xfadeSmoothing, sampleRate);

    // prepare curves
    getSCurve();
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

    if (region->isOscillator()) {
        const WavetableMulti* wave = nullptr;
        if (!region->isGenerator())
            wave = resources.wavePool.getFileWave(region->sampleId->filename());
        else {
            switch (hash(region->sampleId->filename())) {
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
        }
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
        if (!currentPromise) {
            switchState(State::cleanMeUp);
            return;
        }
        updateLoopInformation();
        speedRatio = static_cast<float>(currentPromise->information.sampleRate / this->sampleRate);
        sourcePosition = region->getOffset(resources.filePool.getOversamplingFactor());
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

    for (unsigned i = 0; i < region->filters.size(); ++i) {
        filters[i].setup(*region, i, triggerEvent.number, triggerEvent.value);
    }

    for (unsigned i = 0; i < region->equalizers.size(); ++i) {
        equalizers[i].setup(*region, i, triggerEvent.value);
    }

    triggerDelay = delay;
    initialDelay = delay + static_cast<int>(region->getDelay() * sampleRate);
    baseFrequency = resources.tuning.getFrequencyOfKey(triggerEvent.number);
    bendStepFactor = centsFactor(region->bendStep);
    bendSmoother.setSmoothing(region->bendSmooth, sampleRate);
    bendSmoother.reset(centsFactor(region->getBendInCents(resources.midiState.getPitchBend())));

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

    if (!region->flexAmpEG) {
        if (egAmplitude.getRemainingDelay() > delay)
            switchState(State::cleanMeUp);
    }
    else {
        if (flexEGs[*region->flexAmpEG]->getRemainingDelay() > static_cast<unsigned>(delay))
            switchState(State::cleanMeUp);
    }

    resources.modMatrix.releaseVoice(id, region->getId(), delay);
}

void sfz::Voice::off(int delay, bool fast) noexcept
{
    if (!region->flexAmpEG) {
        if (region->offMode == SfzOffMode::fast || fast) {
            egAmplitude.setReleaseTime(Default::offTime);
        } else if (region->offMode == SfzOffMode::time) {
            egAmplitude.setReleaseTime(region->offTime);
        }
    }
    else {
        // TODO(jpc): Flex AmpEG
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

    for (auto& filter : filters)
        filter.setSampleRate(sampleRate);

    for (auto& eq : equalizers)
        eq.setSampleRate(sampleRate);

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
        if (region->isOscillator())
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

    if (!region->flexAmpEG) {
        if (!egAmplitude.isSmoothing())
            switchState(State::cleanMeUp);
    }
    else {
        if (flexEGs[*region->flexAmpEG]->isFinished())
            switchState(State::cleanMeUp);
    }

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

    // Amplitude EG
    absl::Span<const float> ampegOut(mm.getModulation(masterAmplitudeTarget), numSamples);
    ASSERT(ampegOut.data());
    copy(ampegOut, modulationSpan);

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
    for (unsigned i = 0; i < region->filters.size(); ++i) {
        filters[i].process(inputChannel, outputChannel, numSamples);
    }

    for (unsigned i = 0; i < region->equalizers.size(); ++i) {
        equalizers[i].process(inputChannel, outputChannel, numSamples);
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

    for (unsigned i = 0; i < region->filters.size(); ++i) {
        filters[i].process(inputChannels, outputChannels, numSamples);
    }

    for (unsigned i = 0; i < region->equalizers.size(); ++i) {
        equalizers[i].process(inputChannels, outputChannels, numSamples);
    }
}

void sfz::Voice::fillWithData(AudioSpan<float> buffer) noexcept
{
    const auto numSamples = buffer.getNumFrames();
    if (numSamples == 0)
        return;

    if (!currentPromise) {
        DBG("[Voice] Missing promise during fillWithData");
        return;
    }

    auto source = currentPromise->getData();

    // calculate interpolation data
    //   indices: integral position in the source audio
    //   coeffs: fractional position normalized 0-1
    auto coeffs = resources.bufferPool.getBuffer(numSamples);
    auto indices = resources.bufferPool.getIndexBuffer(numSamples);
    if (!indices || !coeffs)
        return;
    {
        auto jumps = resources.bufferPool.getBuffer(numSamples);
        if (!jumps)
            return;

        fill(*jumps, pitchRatio * speedRatio);
        pitchEnvelope(*jumps);

        jumps->front() += floatPositionOffset;
        cumsum<float>(*jumps, *jumps);
        sfzInterpolationCast<float>(*jumps, *indices, *coeffs);
        add1<int>(sourcePosition, *indices);
    }

    // calculate loop characteristics
    const auto loop = this->loop;
    const bool isLooping = region->shouldLoop()
        && (static_cast<size_t>(loop.end) < source.getNumFrames());

    /*
               loop start             loop end
                   v                      |
                  /|---------------|\     |
                /  |               |  \   |
              /    |               |    \ v
            /------|---------------|------\
            ^                      ^
        xfin start            xfout start
            <------>               <------>
           xfade size             xfade size
     */

    // loop crossfade partitioning
    absl::Span<int> partitionStarts;
    absl::Span<int> partitionTypes;
    unsigned numPartitions = 0;
    enum PartitionType { kPartitionNormal, kPartitionLoopXfade };

    SpanHolder<absl::Span<int>> partitionBuffers[2];
    if (!isLooping) {
        static const int starts[1] = { 0 };
        static const int types[1] = { kPartitionNormal };
        partitionStarts = absl::MakeSpan(const_cast<int*>(starts), 1);
        partitionTypes = absl::MakeSpan(const_cast<int*>(types), 1);
        numPartitions = 1;
    } else {
        for (auto& buf : partitionBuffers) {
            buf = resources.bufferPool.getIndexBuffer(numSamples);
            if (!buf)
                return;
        }
        partitionStarts = *partitionBuffers[0];
        partitionTypes = *partitionBuffers[1];
        // Note: partitions will be alternance of Normal/Xfade
        //       computed along with index processing below
    }

    // index preprocessing for loops
    if (isLooping) {
        int oldIndex {};
        int oldPartitionType {};
        for (unsigned i = 0; i < numSamples; ++i) {
            int index = (*indices)[i];

            // wrap indices post loop-entry around the loop segment
            int wrappedIndex = (index <= loop.end) ? index :
                (loop.start + (index - loop.start) % loop.size);
            (*indices)[i] = wrappedIndex;

            // identify the partition this index is in
            bool xfading = wrappedIndex >= loop.start && wrappedIndex >= loop.xfOutStart;
            int partitionType = xfading ? kPartitionLoopXfade : kPartitionNormal;
            // if looping or entering a different type, start a new partition
            bool start = i == 0 || wrappedIndex < oldIndex || partitionType != oldPartitionType;
            if (start) {
                partitionStarts[numPartitions] = i;
                partitionTypes[numPartitions] = partitionType;
                ++numPartitions;
            }

            oldIndex = wrappedIndex;
            oldPartitionType = partitionType;
        }
    }
    // index preprocessing for one-shots
    else {
        // cut short the voice at the instant of reaching end of sample
        const auto sampleEnd = min(
            static_cast<int>(currentPromise->information.end),
            static_cast<int>(source.getNumFrames())
        ) - 1;
        for (unsigned i = 0; i < numSamples; ++i) {
            if ((*indices)[i] >= sampleEnd) {
#ifndef NDEBUG
                // Check for underflow
                if (source.getNumFrames() - 1 < currentPromise->information.end) {
                    DBG("[sfizz] Underflow: source available samples "
                        << source.getNumFrames() << "/"
                        << currentPromise->information.end
                        << " for sample " << *region->sampleId);
                }
#endif
                if (!region->flexAmpEG) {
                    egAmplitude.setReleaseTime(0.0f);
                    egAmplitude.startRelease(i);
                }
                else {
                    // TODO(jpc): Flex AmpEG
                    flexEGs[*region->flexAmpEG]->release(i);
                }
                fill<int>(indices->subspan(i), sampleEnd);
                fill<float>(coeffs->subspan(i), 1.0f);
                break;
            }
        }
    }

    // interpolation processing
    const int quality = getCurrentSampleQuality();

    for (unsigned ptNo = 0; ptNo < numPartitions; ++ptNo) {
        // current partition
        const int ptType = partitionTypes[ptNo];
        const unsigned ptStart = partitionStarts[ptNo];
        const unsigned ptNextStart = (ptNo + 1 < numPartitions) ? partitionStarts[ptNo + 1] : numSamples;
        const unsigned ptSize = ptNextStart - ptStart;

        // partition spans
        AudioSpan<float> ptBuffer = buffer.subspan(ptStart, ptSize);
        absl::Span<const int> ptIndices = indices->subspan(ptStart, ptSize);
        absl::Span<const float> ptCoeffs = coeffs->subspan(ptStart, ptSize);

        fillInterpolatedWithQuality<false>(
            source, ptBuffer, ptIndices, ptCoeffs, {}, quality);

        if (ptType == kPartitionLoopXfade) {
            auto xfTemp1 = resources.bufferPool.getBuffer(numSamples);
            auto xfTemp2 = resources.bufferPool.getBuffer(numSamples);
            auto xfIndicesTemp = resources.bufferPool.getIndexBuffer(numSamples);
            if (!xfTemp1 || !xfTemp2 || !xfIndicesTemp)
                return;

            absl::Span<float> xfCurvePos = xfTemp1->first(ptSize);

            // compute crossfade positions
            for (unsigned i = 0; i < ptSize; ++i) {
                float pos = ptIndices[i] + ptCoeffs[i];
                xfCurvePos[i] = (pos - loop.xfOutStart) / loop.xfSize;
            }

            //----------------------------------------------------------------//
            // Crossfade Out
            //   -> fade out signal nearing the loop end
            {
                // compute out curve
                absl::Span<float> xfCurve = xfTemp2->first(ptSize);
                IF_CONSTEXPR (config::loopXfadeCurve == 2) {
                    const Curve& xfIn = getSCurve();
                    for (unsigned i = 0; i < ptSize; ++i)
                        xfCurve[i] = xfIn.evalNormalized(1.0f - xfCurvePos[i]);
                }
                else IF_CONSTEXPR (config::loopXfadeCurve == 1) {
                    const Curve& xfOut = resources.curves.getCurve(6);
                    for (unsigned i = 0; i < ptSize; ++i)
                        xfCurve[i] = xfOut.evalNormalized(xfCurvePos[i]);
                }
                else IF_CONSTEXPR (config::loopXfadeCurve == 0) {
                    // TODO(jpc) vectorize this
                    for (unsigned i = 0; i < ptSize; ++i)
                        xfCurve[i] = clamp(1.0f - xfCurvePos[i], 0.0f, 1.0f);
                }
                // apply out curve
                // (scalar fallback: buffer and curve not aligned)
                size_t numChannels = ptBuffer.getNumChannels();
                for (size_t c = 0; c < numChannels; ++c) {
                    absl::Span<float> channel = ptBuffer.getSpan(c);
                    for (unsigned i = 0; i < ptSize; ++i)
                        channel[i] *= xfCurve[i];
                }
            }
            //----------------------------------------------------------------//
            // Crossfade In
            //   -> fade in signal preceding the loop start
            {
                // compute indices of the crossfade input segment
                absl::Span<int> xfInIndices = xfIndicesTemp->first(ptSize);
                absl::c_copy(ptIndices, xfInIndices.begin());
                subtract1(loop.xfOutStart - loop.xfInStart, xfInIndices);

                // disregard the segment whose indices have been pushed
                // into the negatives, take these virtually as zeroes.
                unsigned applyOffset = 0;
                while (applyOffset < ptSize && xfInIndices[applyOffset] < 0)
                    ++applyOffset;
                unsigned applySize = ptSize - applyOffset;

                // offset the indices and coeffs
                xfInIndices = xfInIndices.subspan(applyOffset);
                absl::Span<const float> xfInCoeffs = ptCoeffs.subspan(applyOffset);
                // offset the curve positions
                absl::Span<float> xfInCurvePos = xfCurvePos.subspan(applyOffset);
                // offset the output buffer
                AudioSpan<float> xfInBuffer = ptBuffer.subspan(applyOffset);

                // compute in curve
                absl::Span<float> xfCurve = xfTemp2->first(applySize);
                IF_CONSTEXPR (config::loopXfadeCurve == 2) {
                    const Curve& xfIn = getSCurve();
                    for (unsigned i = 0; i < applySize; ++i)
                        xfCurve[i] = xfIn.evalNormalized(xfInCurvePos[i]);
                }
                else IF_CONSTEXPR (config::loopXfadeCurve == 1) {
                    const Curve& xfIn = resources.curves.getCurve(5);
                    for (unsigned i = 0; i < applySize; ++i)
                        xfCurve[i] = xfIn.evalNormalized(xfInCurvePos[i]);
                }
                else IF_CONSTEXPR (config::loopXfadeCurve == 0) {
                    // TODO(jpc) vectorize this
                    for (unsigned i = 0; i < applySize; ++i)
                        xfCurve[i] = clamp(xfInCurvePos[i], 0.0f, 1.0f);
                }
                // apply in curve
                fillInterpolatedWithQuality<true>(
                    source, xfInBuffer, xfInIndices, xfInCoeffs, xfCurve, quality);
            }
        }
    }

    sourcePosition = indices->back();
    floatPositionOffset = coeffs->back();

#if 1
    ASSERT(!hasNanInf(buffer.getConstSpan(0)));
    ASSERT(!hasNanInf(buffer.getConstSpan(1)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(0)));
    SFIZZ_CHECK(isReasonableAudio(buffer.getConstSpan(1)));
#endif
}

template <sfz::InterpolatorModel M, bool Adding>
void sfz::Voice::fillInterpolated(
    const sfz::AudioSpan<const float>& source, const sfz::AudioSpan<float>& dest,
    absl::Span<const int> indices, absl::Span<const float> coeffs,
    absl::Span<const float> addingGains)
{
    auto* ind = indices.data();
    auto* coeff = coeffs.data();
    auto* addingGain = addingGains.data();
    auto leftSource = source.getConstSpan(0);
    auto left = dest.getChannel(0);
    if (source.getNumChannels() == 1) {
        while (ind < indices.end()) {
            auto output = sfz::interpolate<M>(&leftSource[*ind], *coeff);
            IF_CONSTEXPR(Adding) {
                float g = *addingGain++;
                *left += g * output;
            }
            else
                *left = output;
            incrementAll(ind, left, coeff);
        }
    } else {
        auto right = dest.getChannel(1);
        auto rightSource = source.getConstSpan(1);
        while (ind < indices.end()) {
            auto leftOutput = sfz::interpolate<M>(&leftSource[*ind], *coeff);
            auto rightOutput = sfz::interpolate<M>(&rightSource[*ind], *coeff);
            IF_CONSTEXPR(Adding) {
                float g = *addingGain++;
                *left += g * leftOutput;
                *right += g * rightOutput;
            }
            else {
                *left = leftOutput;
                *right = rightOutput;
            }
            incrementAll(ind, left, right, coeff);
        }
    }
}

template <bool Adding>
void sfz::Voice::fillInterpolatedWithQuality(
    const sfz::AudioSpan<const float>& source, const sfz::AudioSpan<float>& dest,
    absl::Span<const int> indices, absl::Span<const float> coeffs,
    absl::Span<const float> addingGains, int quality)
{
    switch (quality) {
    default:
        if (quality > 2)
            goto high; // TODO sinc, not implemented
        // fall through
    case 1:
        {
            constexpr auto itp = kInterpolatorLinear;
            fillInterpolated<itp, Adding>(source, dest, indices, coeffs, addingGains);
        }
        break;
    case 2: high:
        {
#if 1
            // B-spline response has faster decay of aliasing, but not zero-crossings at integer positions
            constexpr auto itp = kInterpolatorBspline3;
#else
            // Hermite polynomial
            constexpr auto itp = kInterpolatorHermite3;
#endif
            fillInterpolated<itp, Adding>(source, dest, indices, coeffs, addingGains);
        }
        break;
    }
}

const sfz::Curve& sfz::Voice::getSCurve()
{
    static const Curve curve = []() -> Curve {
        constexpr unsigned N = Curve::NumValues;
        float values[N];
        for (unsigned i = 0; i < N; ++i) {
            double x = i / static_cast<double>(N - 1);
            values[i] = (1.0 - std::cos(M_PI * x)) * 0.5;
        }
        return Curve::buildFromPoints(values);
    }();
    return curve;
}

void sfz::Voice::fillWithGenerator(AudioSpan<float> buffer) noexcept
{
    const auto leftSpan = buffer.getSpan(0);
    const auto rightSpan  = buffer.getSpan(1);

    if (region->sampleId->filename() == "*noise") {
        auto gen = [&]() {
            return uniformNoiseDist(Random::randomGenerator);
        };
        absl::c_generate(leftSpan, gen);
        absl::c_generate(rightSpan, gen);
    } else if (region->sampleId->filename() == "*gnoise") {
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

        auto detuneSpan = resources.bufferPool.getBuffer(numFrames);
        if (!detuneSpan)
            return;

        const int oscillatorMode = region->oscillatorMode;
        const int oscillatorMulti = region->oscillatorMulti;

        if (oscillatorMode <= 0 && oscillatorMulti < 2) {
            // single oscillator
            auto tempSpan = resources.bufferPool.getBuffer(numFrames);
            if (!tempSpan)
                return;

            WavetableOscillator& osc = waveOscillators[0];
            fill(*detuneSpan, 1.0f);
            osc.processModulated(frequencies->data(), detuneSpan->data(), tempSpan->data(), buffer.getNumFrames());
            copy<float>(*tempSpan, leftSpan);
            copy<float>(*tempSpan, rightSpan);
        }
        else if (oscillatorMode <= 0 && oscillatorMulti >= 3) {
            // unison oscillator
            auto tempSpan = resources.bufferPool.getBuffer(numFrames);
            auto tempLeftSpan = resources.bufferPool.getBuffer(numFrames);
            auto tempRightSpan = resources.bufferPool.getBuffer(numFrames);
            if (!tempSpan || !tempLeftSpan || !tempRightSpan)
                return;

            const float* detuneMod = resources.modMatrix.getModulation(oscillatorDetuneTarget);
            for (unsigned u = 0, uSize = waveUnisonSize; u < uSize; ++u) {
                WavetableOscillator& osc = waveOscillators[u];
                if (!detuneMod)
                    fill(*detuneSpan, waveDetuneRatio[u]);
                else {
                    for (size_t i = 0; i < numFrames; ++i)
                        (*detuneSpan)[i] = centsFactor(detuneMod[i]);
                    applyGain1(waveDetuneRatio[u], *detuneSpan);
                }
                osc.processModulated(frequencies->data(), detuneSpan->data(), tempSpan->data(), numFrames);
                if (u == 0) {
                    applyGain1<float>(waveLeftGain[u], *tempSpan, *tempLeftSpan);
                    applyGain1<float>(waveRightGain[u], *tempSpan, *tempRightSpan);
                }
                else {
                    multiplyAdd1<float>(waveLeftGain[u], *tempSpan, *tempLeftSpan);
                    multiplyAdd1<float>(waveRightGain[u], *tempSpan, *tempRightSpan);
                }
            }

            copy<float>(*tempLeftSpan, leftSpan);
            copy<float>(*tempRightSpan, rightSpan);
        }
        else {
            // modulated oscillator
            auto tempSpan = resources.bufferPool.getBuffer(numFrames);
            if (!tempSpan)
                return;

            WavetableOscillator& oscCar = waveOscillators[0];
            WavetableOscillator& oscMod = waveOscillators[1];

            // compute the modulator
            auto modulatorSpan = resources.bufferPool.getBuffer(numFrames);
            if (!modulatorSpan)
                return;

            const float* detuneMod = resources.modMatrix.getModulation(oscillatorDetuneTarget);
            if (!detuneMod)
                fill(*detuneSpan, waveDetuneRatio[1]);
            else {
                for (size_t i = 0; i < numFrames; ++i)
                    (*detuneSpan)[i] = centsFactor(detuneMod[i]);
                applyGain1(waveDetuneRatio[1], *detuneSpan);
            }

            oscMod.processModulated(frequencies->data(), detuneSpan->data(), modulatorSpan->data(), numFrames);

            // scale the modulator
            const float oscillatorModDepth = region->oscillatorModDepth;
            if (oscillatorModDepth != 1.0f)
                applyGain1(oscillatorModDepth, *modulatorSpan);
            const float* modDepthMod = resources.modMatrix.getModulation(oscillatorModDepthTarget);
            if (modDepthMod)
                multiplyMul1(0.01f, absl::MakeConstSpan(modDepthMod, numFrames), *modulatorSpan);

            // compute carrier×modulator
            switch (region->oscillatorMode) {
            case 0: // RM synthesis
            default:
                fill(*detuneSpan, 1.0f);
                oscCar.processModulated(frequencies->data(), detuneSpan->data(), tempSpan->data(), buffer.getNumFrames());
                applyGain<float>(*modulatorSpan, *tempSpan);
                break;

            case 1: // PM synthesis
                // Note(jpc): not implemented, just do FM instead
                goto fm_synthesis;
                break;

            case 2: // FM synthesis
            fm_synthesis:
                fill(*detuneSpan, 1.0f);
                multiplyAdd<float>(*modulatorSpan, *frequencies, *frequencies);
                oscCar.processModulated(frequencies->data(), detuneSpan->data(), tempSpan->data(), buffer.getNumFrames());
                break;
            }

            copy<float>(*tempSpan, leftSpan);
            copy<float>(*tempSpan, rightSpan);
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

    resetLoopInformation();

    powerFollower.clear();

    for (auto& filter : filters)
        filter.reset();

    for (auto& eq : equalizers)
        eq.reset();

    removeVoiceFromRing();
}

void sfz::Voice::resetLoopInformation() noexcept
{
    loop.start = 0;
    loop.end = 0;
    loop.size = 0;
    loop.xfSize = 0;
    loop.xfOutStart = 0;
    loop.xfInStart = 0;
}

void sfz::Voice::updateLoopInformation() noexcept
{
    if (!region || !currentPromise)
        return;

    if (!region->shouldLoop())
        return;
    const auto& info = currentPromise->information;
    const auto factor = resources.filePool.getOversamplingFactor();
    const auto rate = info.sampleRate;

    loop.end = static_cast<int>(region->loopEnd(factor));
    loop.start = static_cast<int>(region->loopStart(factor));
    loop.size = loop.end + 1 - loop.start;
    loop.xfSize = static_cast<int>(lroundPositive(region->loopCrossfade * rate));
    loop.xfOutStart = loop.end + 1 - loop.xfSize;
    loop.xfInStart = loop.start - loop.xfSize;
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
    if (followPower)
        return powerFollower.getAveragePower();
    else
        return 0.0f;
}

bool sfz::Voice::releasedOrFree() const noexcept
{
    if (state != State::playing)
        return true;
    if (!region->flexAmpEG)
        return egAmplitude.isReleased();
    else
        return flexEGs[*region->flexAmpEG]->isReleased();
}

void sfz::Voice::setMaxFiltersPerVoice(size_t numFilters)
{
    if (numFilters == filters.size())
        return;

    filters.clear();
    for (unsigned i = 0; i < numFilters; ++i)
        filters.emplace_back(resources);
}

void sfz::Voice::setMaxEQsPerVoice(size_t numFilters)
{
    if (numFilters == equalizers.size())
        return;

    equalizers.clear();
    for (unsigned i = 0; i < numFilters; ++i)
        equalizers.emplace_back(resources);
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

void sfz::Voice::setMaxFlexEGsPerVoice(size_t numFlexEGs)
{
    flexEGs.resize(numFlexEGs);

    for (size_t i = 0; i < numFlexEGs; ++i) {
        auto eg = absl::make_unique<FlexEnvelope>();
        eg->setSampleRate(sampleRate);
        flexEGs[i] = std::move(eg);
    }
}

void sfz::Voice::setPitchEGEnabledPerVoice(bool havePitchEG)
{
    if (havePitchEG)
        egPitch.reset(new ADSREnvelope<float>);
    else
        egPitch.reset();
}

void sfz::Voice::setFilterEGEnabledPerVoice(bool haveFilterEG)
{
    if (haveFilterEG)
        egFilter.reset(new ADSREnvelope<float>);
    else
        egFilter.reset();
}

void sfz::Voice::setupOscillatorUnison()
{
    const int m = region->oscillatorMulti;
    const float d = region->oscillatorDetune;

    // 3-9: unison mode, 1: normal/RM, 2: PM/FM
    if (m < 3 || region->oscillatorMode > 0) {
        waveUnisonSize = 1;
        // carrier
        waveDetuneRatio[0] = 1.0;
        waveLeftGain[0] = 1.0;
        waveRightGain[0] = 1.0;
        // modulator
        const float modDepth = region->oscillatorModDepth;
        waveDetuneRatio[1] = centsFactor(d);
        waveLeftGain[1] = modDepth;
        waveRightGain[1] = modDepth;
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
        waveDetuneRatio[i] = centsFactor(detunes[i]);

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
    masterAmplitudeTarget = mm.findTarget(ModKey::createNXYZ(ModId::MasterAmplitude, region->getId()));
    amplitudeTarget = mm.findTarget(ModKey::createNXYZ(ModId::Amplitude, region->getId()));
    volumeTarget = mm.findTarget(ModKey::createNXYZ(ModId::Volume, region->getId()));
    panTarget = mm.findTarget(ModKey::createNXYZ(ModId::Pan, region->getId()));
    positionTarget = mm.findTarget(ModKey::createNXYZ(ModId::Position, region->getId()));
    widthTarget = mm.findTarget(ModKey::createNXYZ(ModId::Width, region->getId()));
    pitchTarget = mm.findTarget(ModKey::createNXYZ(ModId::Pitch, region->getId()));
    oscillatorDetuneTarget = mm.findTarget(ModKey::createNXYZ(ModId::OscillatorDetune, region->getId()));
    oscillatorModDepthTarget = mm.findTarget(ModKey::createNXYZ(ModId::OscillatorModDepth, region->getId()));
}

void sfz::Voice::enablePowerFollower() noexcept
{
    followPower = true;
    powerFollower.clear();
}

void sfz::Voice::disablePowerFollower() noexcept
{
    followPower = false;
}
