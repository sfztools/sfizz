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
#include "Range.h"
#include <limits>
#include <cstdint>

enum class SfzTrigger { attack, release, release_key, first, legato };
enum class SfzLoopMode { no_loop, one_shot, loop_continuous, loop_sustain };
enum class SfzOffMode { fast, normal };
enum class SfzVelocityOverride { current, previous };
enum class SfzCrossfadeCurve { gain, power };

namespace sfz
{
namespace Default
{
    // The categories match http://sfzformat.com/
    // ******* SFZ 1 *******
    // Sound source: sample playback
    inline constexpr float delay { 0.0 };
    inline constexpr float delayRandom { 0.0 };
    inline constexpr Range<float> delayRange { 0.0, 100.0 };
    inline constexpr uint32_t offset { 0 };
    inline constexpr uint32_t offsetRandom { 0 };
    inline constexpr Range<uint32_t> offsetRange { 0, std::numeric_limits<uint32_t>::max() };
    inline constexpr Range<uint32_t> sampleEndRange { 0, std::numeric_limits<uint32_t>::max() };
    inline constexpr Range<uint32_t> sampleCountRange { 0, std::numeric_limits<uint32_t>::max() };
    inline constexpr SfzLoopMode loopMode { SfzLoopMode::no_loop };
    inline constexpr Range<uint32_t> loopRange { 0, std::numeric_limits<uint32_t>::max() };

    // Instrument setting: voice lifecycle
    inline constexpr uint32_t group { 0 };
    inline constexpr Range<uint32_t> groupRange { 0, std::numeric_limits<uint32_t>::max() };
    inline constexpr SfzOffMode offMode { SfzOffMode::fast };

    // Region logic: key mapping
    inline constexpr Range<uint8_t> keyRange { 0, 127 };
    inline constexpr Range<uint8_t> velocityRange { 0, 127 };

    // Region logic: MIDI conditions
    inline constexpr Range<uint8_t> channelRange { 1, 16 };
    inline constexpr Range<uint8_t> ccRange { 0, 127 };
    inline constexpr uint8_t cc { 0 };
    inline constexpr Range<int> bendRange { -8192, 8192 };
    inline constexpr int bend { 0 };
    inline constexpr SfzVelocityOverride velocityOverride { SfzVelocityOverride::current };

    // Region logic: internal conditions
    inline constexpr Range<float> randRange { 0.0, 1.0 };
    inline constexpr Range<uint8_t> aftertouchRange { 0, 127 };
    inline constexpr uint8_t aftertouch { 0 };
    inline constexpr Range<float> bpmRange { 0.0, 500.0 };
    inline constexpr float bpm { 120.0 };
    inline constexpr uint8_t sequenceLength{ 1 };
    inline constexpr uint8_t sequencePosition{ 1 };
    inline constexpr Range<uint8_t> sequenceRange { 1, 100 };

    // Region logic: Triggers
    inline constexpr SfzTrigger trigger { SfzTrigger::attack };
    inline constexpr Range<uint8_t> ccTriggerValueRange{ 0, 127 };

    // Performance parameters: amplifier
    inline constexpr float volume { -3.0 };
    inline constexpr Range<float> volumeRange { -144.0, 6.0 };
    inline constexpr Range<float> volumeCCRange { -144.0, 6.0 };
    inline constexpr float amplitude { 100.0 };
    inline constexpr Range<float> amplitudeRange { 0.0, 100.0 };
    inline constexpr float pan { 0.0 };
    inline constexpr Range<float> panRange { -100.0, 100.0 };
    inline constexpr Range<float> panCCRange { -200.0, 200.0 };
    inline constexpr float position { 0.0 };
    inline constexpr Range<float> positionRange { -100.0, 100.0 };
    inline constexpr Range<float> positionCCRange { -200.0, 200.0 };
    inline constexpr float width { 0.0 };
    inline constexpr Range<float> widthRange { -100.0, 100.0 };
    inline constexpr Range<float> widthCCRange { -200.0, 200.0 };
    inline constexpr uint8_t ampKeycenter { 60 };
    inline constexpr float ampKeytrack { 0.0 };
    inline constexpr Range<float> ampKeytrackRange { -96, 12 };
    inline constexpr float ampVeltrack { 100.0 };
    inline constexpr Range<float> ampVeltrackRange { -100.0, 100.0 };
    inline constexpr Range<float> ampVelcurveRange { 0.0, 1.0 };
    inline constexpr float ampRandom { 0.0 };
    inline constexpr Range<float> ampRandomRange { 0.0, 24.0 };
    inline constexpr Range<uint8_t> crossfadeKeyInRange { 0, 0 };
    inline constexpr Range<uint8_t> crossfadeKeyOutRange { 127, 127 };
    inline constexpr Range<uint8_t> crossfadeVelInRange { 0, 0 };
    inline constexpr Range<uint8_t> crossfadeVelOutRange { 127, 127 };
    inline constexpr Range<uint8_t> crossfadeCCInRange { 0, 0 };
    inline constexpr Range<uint8_t> crossfadeCCOutRange { 127, 127 };
    inline constexpr SfzCrossfadeCurve crossfadeKeyCurve { SfzCrossfadeCurve::power };
    inline constexpr SfzCrossfadeCurve crossfadeVelCurve { SfzCrossfadeCurve::power };
    inline constexpr SfzCrossfadeCurve crossfadeCCCurve { SfzCrossfadeCurve::power };
    inline constexpr float rtDecay { 0.0f };
    inline constexpr Range<float> rtDecayRange { 0.0f, 200.0f };

     // Performance parameters: pitch
    inline constexpr uint8_t pitchKeycenter { 60 };
    inline constexpr int pitchKeytrack { 100 };
    inline constexpr Range<int> pitchKeytrackRange { -1200, 1200 };
    inline constexpr int pitchRandom { 0 };
    inline constexpr Range<int> pitchRandomRange { 0, 9600 };
    inline constexpr int pitchVeltrack { 0 };
    inline constexpr Range<int> pitchVeltrackRange { -9600, 9600 };
    inline constexpr int transpose { 0 };
    inline constexpr Range<int> transposeRange { -127, 127 };
    inline constexpr int tune { 0 };
    inline constexpr Range<int> tuneRange { -100, 100 };

    // Envelope generators
    inline constexpr float attack { 0 };
    inline constexpr float decay { 0 };
    inline constexpr float delayEG { 0 };
    inline constexpr float hold { 0 };
    inline constexpr float release { 0 };
    inline constexpr float start { 0.0 };
    inline constexpr float sustain { 100.0 };
    inline constexpr float vel2sustain { 0.0 };
    inline constexpr int depth { 0 };
    inline constexpr Range<float> egTimeRange { 0.0, 100.0 };
    inline constexpr Range<float> egPercentRange { 0.0, 100.0 };
    inline constexpr Range<int> egDepthRange { -12000, 12000 };
    inline constexpr Range<float> egOnCCTimeRange { -100.0, 100.0 };
    inline constexpr Range<float> egOnCCPercentRange { -100.0, 100.0 };

    // ***** SFZ v2 ********
    inline constexpr bool checkSustain { true }; // sustain_sw
    inline constexpr bool checkSostenuto { true }; // sostenuto_sw
}
}