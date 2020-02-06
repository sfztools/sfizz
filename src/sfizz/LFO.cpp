// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "LFO.h"
#include <algorithm>
#include <cmath>

namespace sfz {

static const LFO::Control defaultControls;

void LFO::init(double sampleRate)
{
    this->sampleRate = sampleRate;
    control = &defaultControls;
}

void LFO::attachParameters(const Control* ctl)
{
    control = ctl ? ctl : &defaultControls;
}

void LFO::start()
{
    const Control& ctl = *control;

    subPhases.fill(ctl.phase0);

    delayFramesLeft = (unsigned)std::ceil(sampleRate * ctl.delay);

    fadeInPole = std::exp(-1.0 / (ctl.fade * sampleRate));
    fadeInMemory = 0;
}

template <>
inline float LFO::eval<LFO::kTriangle>(float phase)
{
    float y = -4 * phase + 2;
    y = (phase < 0.25f) ? (4 * phase) : y;
    y = (phase > 0.75f) ? (4 * phase - 4) : y;
    return y;
}

static const std::array<float, 1024> tabSine1024 = []() {
    std::array<float, 1024> tab;
    for (int i = 0, n = (int)tab.size(); i < n; ++i)
        tab[i] = std::sin(i * (2.0 * M_PI / n));
    return tab;
}();

template <>
inline float LFO::eval<LFO::kSine>(float phase)
{
    return tabSine1024[unsigned(phase * 1024)];
}

template <>
inline float LFO::eval<LFO::kPulse75>(float phase)
{
    return (phase < 0.75f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFO::kSquare>(float phase)
{
    return (phase < 0.5f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFO::kPulse25>(float phase)
{
    return (phase < 0.25f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFO::kPulse12_5>(float phase)
{
    return (phase < 0.125f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFO::kRamp>(float phase)
{
    return 2 * phase - 1;
}

template <>
inline float LFO::eval<LFO::kSaw>(float phase)
{
    return 1 - 2 * phase;
}

template <LFO::Wave W>
void LFO::processWave(unsigned nth, float* out, unsigned nframes)
{
    const Control& ctl = *control;
    const Control::Sub& sub = ctl.sub[nth];

    float samplePeriod = 1.0f / sampleRate;
    float phase = subPhases[nth];
    float baseFreq = ctl.freq;
    float offset = sub.offset;
    float ratio = sub.ratio;
    float scale = sub.scale;

    for (unsigned i = 0; i < nframes; ++i) {
        out[i] += offset + scale * eval<W>(phase);

        // TODO(jpc) lfoN_count: number of repetitions

        float incrPhase = ratio * samplePeriod * baseFreq;
        phase += incrPhase;
        unsigned numWraps = (int)phase;
        phase -= numWraps;
    }

    subPhases[nth] = phase;
}

void LFO::process(float* out, unsigned nframes)
{
    const Control& ctl = *control;

    std::fill(out, out + nframes, 0.0f);

    unsigned skipFrames = std::min(nframes, delayFramesLeft);
    if (skipFrames > 0) {
        delayFramesLeft -= skipFrames;
        out += skipFrames;
        nframes -= skipFrames;
    }

    for (unsigned i = 0, n = ctl.countSubs; i < n; ++i) {
        switch (ctl.sub[i].wave) {
        case kTriangle:
            processWave<kTriangle>(i, out, nframes);
            break;
        case kSine:
            processWave<kSine>(i, out, nframes);
            break;
        case kPulse75:
            processWave<kPulse75>(i, out, nframes);
            break;
        case kSquare:
            processWave<kSquare>(i, out, nframes);
            break;
        case kPulse25:
            processWave<kPulse25>(i, out, nframes);
            break;
        case kPulse12_5:
            processWave<kPulse12_5>(i, out, nframes);
            break;
        case kRamp:
            processWave<kRamp>(i, out, nframes);
            break;
        case kSaw:
            processWave<kSaw>(i, out, nframes);
            break;
        }
    }

    float fadeInGain = fadeInMemory;
    float fadeInPole = this->fadeInPole;
    for (unsigned i = 0; i < nframes; ++i) {
        out[i] *= fadeInGain;
        fadeInGain = fadeInPole * fadeInGain + (1 - fadeInPole);
    }
    fadeInMemory = fadeInGain;
}

} // namespace sfz
