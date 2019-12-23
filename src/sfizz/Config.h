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

namespace sfz {
enum class Oversampling: int {
    x1 = 1,
    x2 = 2,
    x4 = 4,
    x8 = 8
};

namespace config {
    constexpr float defaultSampleRate { 48000 };
    constexpr int defaultSamplesPerBlock { 1024 };
    constexpr int maxBlockSize { 8192 };
    constexpr int preloadSize { 8192 };
    constexpr int numChannels { 2 };
    constexpr int numBackgroundThreads { 4 };
    constexpr int numVoices { 64 };
    constexpr int maxVoices { 256 };
    constexpr int maxFilePromises { maxVoices * 2 };
    constexpr int sustainCC { 64 };
    constexpr int allSoundOffCC { 120 };
    constexpr int resetCC { 121 };
    constexpr int allNotesOffCC { 123 };
    constexpr int omniOffCC { 124 };
    constexpr int omniOnCC { 125 };
    constexpr int halfCCThreshold { 64 };
    constexpr int centPerSemitone { 100 };
    constexpr float virtuallyZero { 0.00005f };
    constexpr float fastReleaseDuration { 0.01 };
    constexpr char defineCharacter { '$' };
    constexpr Oversampling defaultOversamplingFactor { Oversampling::x1 };
    constexpr float A440 { 440.0 };
    constexpr unsigned powerHistoryLength { 16 };
    constexpr float voiceStealingThreshold { 0.00001 };
    constexpr int numCCs { 143 };
    constexpr int chunkSize { 1024 };
} // namespace config

// Enable or disable SIMD accelerators by default
namespace SIMDConfig {
    constexpr unsigned int defaultAlignment { 16 };
    constexpr bool writeInterleaved { true };
    constexpr bool readInterleaved { true };
    constexpr bool fill { true };
    constexpr bool gain { false };
    constexpr bool divide { false };
    constexpr bool mathfuns { false };
    constexpr bool loopingSFZIndex { true };
    constexpr bool saturatingSFZIndex { true };
    constexpr bool linearRamp { false };
    constexpr bool multiplicativeRamp { true };
    constexpr bool add { false };
    constexpr bool subtract { false };
    constexpr bool multiplyAdd { false };
    constexpr bool copy { false };
    constexpr bool pan { true };
    constexpr bool cumsum { true };
    constexpr bool diff { false };
    constexpr bool sfzInterpolationCast { true };
    constexpr bool mean { false };
    constexpr bool meanSquared { false };
    constexpr bool upsampling { true };
}
} // namespace sfz
