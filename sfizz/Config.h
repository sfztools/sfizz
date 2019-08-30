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

namespace config {
    constexpr float defaultSampleRate { 48000 };
    constexpr int defaultSamplesPerBlock { 1024 };
    constexpr int preloadSize { 8192 * 2 };
    constexpr int numChannels { 2 };
    constexpr int numVoices { 64 };
    constexpr int numLoadingThreads { 4 };
    constexpr int centPerSemitone { 100 };
    constexpr float virtuallyZero { 0.00005f };
    constexpr float fastReleaseDuration { 0.01 };
    constexpr char defineCharacter { '$' };
    constexpr int oversamplingFactor { 2 };
    constexpr float A440 { 440.0 };
} // namespace config

} // namespace sfz

namespace SIMDConfig {
    constexpr unsigned int defaultAlignment { 16 };
    constexpr bool writeInterleaved { true };
    constexpr bool readInterleaved { true };
    constexpr bool fill { true };
    constexpr bool gain { false };
    constexpr bool mathfuns { false };
    constexpr bool loopingSFZIndex { true };
    constexpr bool saturatingSFZIndex { true };
    constexpr bool linearRamp { false };
    constexpr bool multiplicativeRamp { true };
    constexpr bool add { false };
}