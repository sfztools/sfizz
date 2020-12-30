// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
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
#include "Range.h"
#include "Config.h"
#include <limits>
#include <cstdint>

enum class SfzTrigger { attack = 0, release, release_key, first, legato };
enum class SfzLoopMode { no_loop = 0, one_shot, loop_continuous, loop_sustain };
enum class SfzOffMode { fast = 0, normal, time };
enum class SfzVelocityOverride { current = 0, previous };
enum class SfzCrossfadeCurve { gain = 0, power };
enum class SfzSelfMask { mask = 0, dontMask };

namespace sfz
{

enum class OscillatorEnabled { Auto = -1, Off = 0, On = 1 };

enum OpcodeFlags : int {
    kIgnoreOOB = 1,
    kEnforceLowerBound = 1 << 1,
    kEnforceUpperBound = 1 << 2,
    kCanBeNote = 1 << 3,
};

template<class T>
struct OpcodeSpec
{
    T value;
    Range<T> bounds;
    int flags;
};

namespace Default
{
    extern const OpcodeSpec<float> delay;
    extern const OpcodeSpec<float> delayRandom;
    extern const OpcodeSpec<int64_t> offset;
    extern const OpcodeSpec<int64_t> offsetMod;
    extern const OpcodeSpec<int64_t> offsetRandom;
    extern const OpcodeSpec<uint32_t> sampleEnd;
    extern const OpcodeSpec<uint32_t> sampleCount;
    extern const OpcodeSpec<uint32_t> loopRange;
    extern const OpcodeSpec<float> loopCrossfade;
    extern const OpcodeSpec<float> oscillatorPhase;
    extern const OpcodeSpec<OscillatorEnabled> oscillator;
    extern const OpcodeSpec<int> oscillatorMode;
    extern const OpcodeSpec<int> oscillatorMulti;
    extern const OpcodeSpec<float> oscillatorDetune;
    extern const OpcodeSpec<float> oscillatorDetuneMod;
    extern const OpcodeSpec<float> oscillatorModDepth;
    extern const OpcodeSpec<float> oscillatorModDepthMod;
    extern const OpcodeSpec<int> oscillatorQuality;
    extern const OpcodeSpec<uint32_t> group;
    extern const OpcodeSpec<float> offTime;
    extern const OpcodeSpec<uint32_t> polyphony;
    extern const OpcodeSpec<uint32_t> notePolyphony;
    extern const OpcodeSpec<uint8_t> key;
    extern const OpcodeSpec<uint8_t> midi7;
    extern const OpcodeSpec<float> float7;
    extern const OpcodeSpec<float> bend;
    extern const OpcodeSpec<float> normalized;
    extern const OpcodeSpec<float> bipolar;
    extern const OpcodeSpec<uint16_t> ccNumber;
    extern const OpcodeSpec<uint8_t> curveCC;
    extern const OpcodeSpec<uint8_t> smoothCC;
    extern const OpcodeSpec<uint8_t> sustainCC;
    extern const OpcodeSpec<bool> checkSustain;
    extern const OpcodeSpec<bool> checkSostenuto;
    extern const OpcodeSpec<float> sustainThreshold;
    extern const OpcodeSpec<float> bpm;
    extern const OpcodeSpec<uint8_t> sequence;
    extern const OpcodeSpec<float> volume;
    extern const OpcodeSpec<float> volumeMod;
    extern const OpcodeSpec<float> amplitude;
    extern const OpcodeSpec<float> amplitudeMod;
    extern const OpcodeSpec<float> pan;
    extern const OpcodeSpec<float> panMod;
    extern const OpcodeSpec<float> position;
    extern const OpcodeSpec<float> positionMod;
    extern const OpcodeSpec<float> width;
    extern const OpcodeSpec<float> widthMod;
    extern const OpcodeSpec<uint8_t> crossfadeIn;
    extern const OpcodeSpec<float> crossfadeInNorm;
    extern const OpcodeSpec<uint8_t> crossfadeOut;
    extern const OpcodeSpec<float> crossfadeOutNorm;
    extern const OpcodeSpec<float> ampKeytrack;
    extern const OpcodeSpec<float> ampVeltrack;
    extern const OpcodeSpec<float> ampVelcurve;
    extern const OpcodeSpec<float> ampRandom;
    extern const OpcodeSpec<bool> rtDead;
    extern const OpcodeSpec<float> rtDecay;
    extern const OpcodeSpec<float> filterCutoff;
    extern const OpcodeSpec<float> filterCutoffMod;
    extern const OpcodeSpec<float> filterResonance;
    extern const OpcodeSpec<float> filterResonanceMod;
    extern const OpcodeSpec<float> filterGain;
    extern const OpcodeSpec<float> filterGainMod;
    extern const OpcodeSpec<float> filterRandom;
    extern const OpcodeSpec<int> filterKeytrack;
    extern const OpcodeSpec<int> filterVeltrack;
    extern const OpcodeSpec<float> eqBandwidth;
    extern const OpcodeSpec<float> eqBandwidthMod;
    extern const OpcodeSpec<float> eqFrequency;
    extern const OpcodeSpec<float> eqFrequencyMod;
    extern const OpcodeSpec<float> eqGain;
    extern const OpcodeSpec<float> eqGainMod;
    extern const OpcodeSpec<float> eqVel2Frequency;
    extern const OpcodeSpec<float> eqVel2Gain;
    extern const OpcodeSpec<int> pitchKeytrack;
    extern const OpcodeSpec<float> pitchRandom;
    extern const OpcodeSpec<int> pitchVeltrack;
    extern const OpcodeSpec<int> transpose;
    extern const OpcodeSpec<float> pitch;
    extern const OpcodeSpec<float> pitchMod;
    extern const OpcodeSpec<float> bendUp;
    extern const OpcodeSpec<float> bendDown;
    extern const OpcodeSpec<float> bendStep;
    extern const OpcodeSpec<float> lfoFreq;
    extern const OpcodeSpec<float> lfoFreqMod;
    extern const OpcodeSpec<float> lfoBeats;
    extern const OpcodeSpec<float> lfoBeatsMod;
    extern const OpcodeSpec<float> lfoPhase;
    extern const OpcodeSpec<float> lfoDelay;
    extern const OpcodeSpec<float> lfoFade;
    extern const OpcodeSpec<unsigned> lfoCount;
    extern const OpcodeSpec<unsigned> lfoSteps;
    extern const OpcodeSpec<float> lfoStepX;
    extern const OpcodeSpec<int> lfoWave;
    extern const OpcodeSpec<float> lfoOffset;
    extern const OpcodeSpec<float> lfoRatio;
    extern const OpcodeSpec<float> lfoScale;
    extern const OpcodeSpec<float> egTime;
    extern const OpcodeSpec<float> egRelease;
    extern const OpcodeSpec<float> egTimeMod;
    extern const OpcodeSpec<float> egPercent;
    extern const OpcodeSpec<float> egPercentMod;
    extern const OpcodeSpec<float> egDepth;
    extern const OpcodeSpec<float> egVel2Depth;
    extern const OpcodeSpec<bool> flexEGAmpeg;
    extern const OpcodeSpec<int> flexEGDynamic;
    extern const OpcodeSpec<int> flexEGSustain;
    extern const OpcodeSpec<float> flexEGPointTime;
    extern const OpcodeSpec<float> flexEGPointLevel;
    extern const OpcodeSpec<float> flexEGPointShape;
    extern const OpcodeSpec<int> sampleQuality;
    extern const OpcodeSpec<int> octaveOffset;
    extern const OpcodeSpec<int> noteOffset;
    extern const OpcodeSpec<float> effect;
    extern const OpcodeSpec<int> apanWaveform;
    extern const OpcodeSpec<float> apanFrequency;
    extern const OpcodeSpec<float> apanPhase;
    extern const OpcodeSpec<float> apanLevel;
    extern const OpcodeSpec<float> distoTone;
    extern const OpcodeSpec<float> distoDepth;
    extern const OpcodeSpec<unsigned> distoStages;
    extern const OpcodeSpec<float> compAttack;
    extern const OpcodeSpec<float> compRelease;
    extern const OpcodeSpec<float> compThreshold;
    extern const OpcodeSpec<bool> compSTLink;
    extern const OpcodeSpec<float> compRatio;
    extern const OpcodeSpec<float> compGain;
    extern const OpcodeSpec<float> fverbSize;
    extern const OpcodeSpec<float> fverbPredelay;
    extern const OpcodeSpec<float> fverbTone;
    extern const OpcodeSpec<float> fverbDamp;
    extern const OpcodeSpec<float> gateAttack;
    extern const OpcodeSpec<float> gateRelease;
    extern const OpcodeSpec<bool> gateSTLink;
    extern const OpcodeSpec<float> gateHold;
    extern const OpcodeSpec<float> gateThreshold;
    extern const OpcodeSpec<float> lofiBitred;
    extern const OpcodeSpec<float> lofiDecim;
    extern const OpcodeSpec<float> rectify;
    extern const OpcodeSpec<unsigned> stringsNumber;

    // Default/max count for objects
    constexpr int numEQs { 3 };
    constexpr int numFilters { 2 };
    constexpr int numFlexEGs { 4 };
    constexpr int numFlexEGPoints { 8 };
    constexpr int numLFOs { 4 };
    constexpr int numLFOSubs { 2 };
    constexpr int numLFOSteps { 8 };
    constexpr int maxDistoStages { 4 };
    constexpr unsigned maxStrings { 88 };

    // Default values for enums
    constexpr SfzTrigger trigger { SfzTrigger::attack };
    constexpr SfzOffMode offMode { SfzOffMode::fast };
    constexpr SfzVelocityOverride velocityOverride { SfzVelocityOverride::current };
    constexpr SfzSelfMask selfMask { SfzSelfMask::mask };
    constexpr SfzCrossfadeCurve crossfadeKeyCurve { SfzCrossfadeCurve::power };
    constexpr SfzCrossfadeCurve crossfadeVelCurve { SfzCrossfadeCurve::power };
    constexpr SfzCrossfadeCurve crossfadeCCCurve { SfzCrossfadeCurve::power };

    // Default values for ranges
    constexpr Range<uint8_t> crossfadeKeyInRange { 0, 0 };
    constexpr Range<uint8_t> crossfadeKeyOutRange { 127, 127 };
    constexpr Range<float> crossfadeVelInRange { 0.0f, 0.0f };
    constexpr Range<float> crossfadeVelOutRange { 1.0f, 1.0f };
    constexpr Range<float> crossfadeCCInRange { 0.0f, 0.0f };
    constexpr Range<float> crossfadeCCOutRange { 1.0f, 1.0f };

    // Various defaut values
    // e.g. "additional" or multiple defautl values
    constexpr int freewheelingQuality { 10 };
    constexpr float globalVolume { -7.35f };
    constexpr float defaultEQFreq [numEQs] { 50.0f, 500.0f, 5000.0f };
} // namespace Default

} // namespace sfz
