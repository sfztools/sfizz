// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "LFO.h"
#include "LFODescription.h"
#include "BeatClock.h"
#include "BufferPool.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "Config.h"
#include "modulations/ModMatrix.h"
#include "modulations/ModKey.h"
#include "modulations/ModId.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace sfz {

struct LFO::Impl {
    explicit Impl(BufferPool& bufferPool, BeatClock* beatClock, ModMatrix* modMatrix)
        : bufferPool_(bufferPool),
          beatClock_(beatClock),
          modMatrix_(modMatrix),
          sampleRate_(config::defaultSampleRate),
          desc_(&LFODescription::getDefault())
    {
    }

    BufferPool& bufferPool_;
    BeatClock* beatClock_ = nullptr;
    ModMatrix* modMatrix_ = nullptr;
    float sampleRate_ = 0;

    // control
    const LFODescription* desc_ = nullptr;
    ModMatrix::TargetId beatsModId_ {};
    ModMatrix::TargetId freqModId_ {};

    // state
    size_t delayFramesLeft_ = 0;
    float fadePosition_ = 0;
    std::array<float, config::maxLFOSubs> subPhases_ {{}};
    std::array<float, config::maxLFOSubs> sampleHoldMem_ {{}};
    std::array<int, config::maxLFOSubs> sampleHoldState_ {{}};
};

LFO::LFO(BufferPool& bufferPool, BeatClock* beatClock, ModMatrix* modMatrix)
    : impl_(new Impl(bufferPool, beatClock, modMatrix))
{
}

LFO::~LFO()
{
}

void LFO::setSampleRate(double sampleRate)
{
    impl_->sampleRate_ = sampleRate;
}

void LFO::configure(const LFODescription* desc)
{
    impl_->desc_ = desc ? desc : &LFODescription::getDefault();
}

void LFO::start(unsigned triggerDelay)
{
    Impl& impl = *impl_;
    ModMatrix* modMatrix = impl.modMatrix_;
    const LFODescription& desc = *impl.desc_;
    const float sampleRate = impl.sampleRate_;

    if (modMatrix) {
        impl.beatsModId_ = modMatrix->findTarget(desc.beatsKey);
        impl.freqModId_ = modMatrix->findTarget(desc.freqKey);
    }

    impl.subPhases_.fill(0.0f);
    impl.sampleHoldMem_.fill(0.0f);
    impl.sampleHoldState_.fill(0);

    const float delay = desc.delay;
    size_t delayFrames = (delay > 0) ? static_cast<size_t>(std::ceil(sampleRate * delay)) : 0u;
    impl.delayFramesLeft_ = triggerDelay + delayFrames;

    impl.fadePosition_ = (desc.fade > 0) ? 0.0f : 1.0f;
}

template <>
inline float LFO::eval<LFOWave::Triangle>(float phase)
{
    float y = -4 * phase + 2;
    y = (phase < 0.25f) ? (4 * phase) : y;
    y = (phase > 0.75f) ? (4 * phase - 4) : y;
    return y;
}

template <>
inline float LFO::eval<LFOWave::Sine>(float phase)
{
    float x = phase + phase - 1;
    return -4 * x * (1 - std::fabs(x));
}

// Pulse and Square levels
static constexpr float loPulse = 0.0f; // 0 in ARIA, -1 in Cakewalk
static constexpr float hiPulse = 1.0f;

template <>
inline float LFO::eval<LFOWave::Pulse75>(float phase)
{
    return (phase < 0.75f) ? hiPulse : loPulse;
}

template <>
inline float LFO::eval<LFOWave::Square>(float phase)
{
    return (phase < 0.5f) ? hiPulse : loPulse;
}

template <>
inline float LFO::eval<LFOWave::Pulse25>(float phase)
{
    return (phase < 0.25f) ? hiPulse : loPulse;
}

template <>
inline float LFO::eval<LFOWave::Pulse12_5>(float phase)
{
    return (phase < 0.125f) ? hiPulse : loPulse;
}

template <>
inline float LFO::eval<LFOWave::Ramp>(float phase)
{
    return 2 * phase - 1;
}

template <>
inline float LFO::eval<LFOWave::Saw>(float phase)
{
    return 1 - 2 * phase;
}

template <LFOWave W>
void LFO::processWave(unsigned nth, absl::Span<float> out, const float* phaseIn)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    const float offset = sub.offset;
    const float scale = sub.scale;

    for (size_t i = 0; i < numFrames; ++i) {
        float phase = phaseIn[i];
        out[i] += offset + scale * eval<W>(phase);
    }
}

template <LFOWave W>
void LFO::processSH(unsigned nth, absl::Span<float> out, const float* phaseIn)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    const float offset = sub.offset;
    const float scale = sub.scale;
    float sampleHoldValue = impl.sampleHoldMem_[nth];
    int sampleHoldState = impl.sampleHoldState_[nth];

    for (size_t i = 0; i < numFrames; ++i) {
        out[i] += offset + scale * sampleHoldValue;

        // TODO(jpc) lfoN_count: number of repetitions

        float phase = phaseIn[i];

        int oldState = sampleHoldState;
        sampleHoldState = phase > 0.5f;

        // value updates twice every period
        if (sampleHoldState != oldState) {
            std::uniform_real_distribution<float> dist(-1.0f, +1.0f);
            sampleHoldValue = dist(Random::randomGenerator);
        }
    }

    impl.sampleHoldMem_[nth] = sampleHoldValue;
    impl.sampleHoldState_[nth] = sampleHoldState;
}

void LFO::processSteps(absl::Span<float> out, const float* phaseIn)
{
    unsigned nth = 0;
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    const LFODescription::StepSequence& seq = *desc.seq;
    const float* steps = seq.steps.data();
    unsigned numSteps = seq.steps.size();

    if (numSteps <= 0)
        return;

    const float offset = sub.offset;
    const float scale = sub.scale;

    for (size_t i = 0; i < numFrames; ++i) {
        float phase = phaseIn[i];
        float step = steps[static_cast<int>(phase * numSteps)];
        out[i] += offset + scale * step;
    }
}

void LFO::process(absl::Span<float> out)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    size_t numFrames = out.size();

    fill(out, 0.0f);

    size_t skipFrames = std::min(numFrames, impl.delayFramesLeft_);
    if (skipFrames > 0) {
        impl.delayFramesLeft_ -= skipFrames;
        out.remove_prefix(skipFrames);
        numFrames -= skipFrames;
    }

    unsigned subno = 0;
    const unsigned countSubs = desc.sub.size();

    if (countSubs < 1)
        return;

    auto phasesTemp = impl.bufferPool_.getBuffer(numFrames);
    if (!phasesTemp) {
        ASSERTFALSE;
        fill(out, 0.0f);
        return;
    }

    absl::Span<float> phases = *phasesTemp;

    if (desc.seq) {
        generatePhase(0, phases);
        processSteps(out, phases.data());
        ++subno;
    }

    for (; subno < countSubs; ++subno) {
        generatePhase(subno, phases);
        switch (desc.sub[subno].wave) {
        case LFOWave::Triangle:
            processWave<LFOWave::Triangle>(subno, out, phases.data());
            break;
        case LFOWave::Sine:
            processWave<LFOWave::Sine>(subno, out, phases.data());
            break;
        case LFOWave::Pulse75:
            processWave<LFOWave::Pulse75>(subno, out, phases.data());
            break;
        case LFOWave::Square:
            processWave<LFOWave::Square>(subno, out, phases.data());
            break;
        case LFOWave::Pulse25:
            processWave<LFOWave::Pulse25>(subno, out, phases.data());
            break;
        case LFOWave::Pulse12_5:
            processWave<LFOWave::Pulse12_5>(subno, out, phases.data());
            break;
        case LFOWave::Ramp:
            processWave<LFOWave::Ramp>(subno, out, phases.data());
            break;
        case LFOWave::Saw:
            processWave<LFOWave::Saw>(subno, out, phases.data());
            break;
        case LFOWave::RandomSH:
            processSH<LFOWave::RandomSH>(subno, out, phases.data());
            break;
        }
    }

    processFadeIn(out);
}

void LFO::processFadeIn(absl::Span<float> out)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const float samplePeriod = 1.0f / impl.sampleRate_;
    size_t numFrames = out.size();

    float fadePosition = impl.fadePosition_;
    if (fadePosition >= 1.0f)
        return;

    const float fadeTime = desc.fade;
    const float fadeStep = samplePeriod / fadeTime;

    for (size_t i = 0; i < numFrames && fadePosition < 1; ++i) {
        out[i] *= fadePosition;
        fadePosition = std::min(1.0f, fadePosition + fadeStep);
    }

    impl.fadePosition_ = fadePosition;
}

void LFO::generatePhase(unsigned nth, absl::Span<float> phases)
{
    Impl& impl = *impl_;
    BufferPool& bufferPool = impl.bufferPool_;
    BeatClock* beatClock = impl.beatClock_;
    ModMatrix* modMatrix = impl.modMatrix_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const float samplePeriod = 1.0f / impl.sampleRate_;
    const float baseFreq = desc.freq;
    const float beats = desc.beats;
    const float phaseOffset = desc.phase0;
    const float ratio = sub.ratio;
    float phase = impl.subPhases_[nth];
    const size_t numFrames = phases.size();

    // TODO(jpc) lfoN_count: number of repetitions

    // modulations
    const float* beatsMod = nullptr;
    const float* freqMod = nullptr;
    if (modMatrix) {
        // Note(jpc) we might switch between beats and frequency, if host
        //           switches play state on and off; continually generate both.
        beatsMod = modMatrix->getModulation(impl.beatsModId_);
        freqMod = modMatrix->getModulation(impl.freqModId_);
    }

    if (beatClock && beatClock->isPlaying() && beats > 0) {
        // generate using the beat clock
        float beatRatio = (ratio > 0) ? (1.0f / ratio) : 0.0f;

        if (!beatsMod)
            beatClock->calculatePhase(beats * beatRatio, phases.data());
        else {
            auto temp = bufferPool.getBuffer(numFrames);
            if (!temp) {
                ASSERTFALSE;
                beatClock->calculatePhase(beats * beatRatio, phases.data());
            }
            else {
                fill(*temp, beats);
                add(absl::MakeConstSpan(beatsMod, numFrames), *temp);
                applyGain1(beatRatio, *temp);
                beatClock->calculatePhaseModulated(temp->data(), phases.data());
            }
        }
    }
    else {
        // generate using the frequency
        if (!freqMod) {
            for (size_t i = 0; i < numFrames; ++i) {
                phases[i] = phase;
                float incr = ratio * samplePeriod * baseFreq;
                phase = wrapPhase(phase + incr);
            }
        }
        else {
            for (size_t i = 0; i < numFrames; ++i) {
                phases[i] = phase;
                float incr = ratio * samplePeriod * (baseFreq + freqMod[i]);
                phase = wrapPhase(phase + incr);
            }
        }
    }

    // apply phase offsets
    for (size_t i = 0; i < numFrames; ++i) {
        float withOffset = phases[i] + phaseOffset;
        withOffset -= (int)withOffset;
        phases[i] = withOffset;
    }

    impl.subPhases_[nth] = phase;
}

} // namespace sfz
