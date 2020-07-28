// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#ifdef _WIN32
// There's a spurious min/max function in MSVC that makes everything go badly...
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#endif
#include "absl/strings/string_view.h"
#include <cstddef>
#include <cstdint>

namespace sfz {
enum class Oversampling: int {
    x1 = 1,
    x2 = 2,
    x4 = 4,
    x8 = 8
};

namespace config {
    constexpr float defaultSampleRate { 48000 };
    constexpr float maxSampleRate { 192000 };
    constexpr int defaultSamplesPerBlock { 1024 };
    constexpr int maxBlockSize { 8192 };
    constexpr int bufferPoolSize { 6 };
    constexpr int stereoBufferPoolSize { 4 };
    constexpr int indexBufferPoolSize { 2 };
    constexpr int preloadSize { 8192 };
    constexpr int loggerQueueSize { 256 };
    constexpr int voiceLoggerQueueSize { 256 };
    constexpr bool loggingEnabled { false };
    constexpr size_t numChannels { 2 };
    constexpr int numBackgroundThreads { 4 };
    constexpr int numVoices { 64 };
    constexpr unsigned maxVoices { 256 };
    constexpr unsigned smoothingSteps { 512 };
    constexpr uint8_t xfadeSmoothing { 5 };
    constexpr uint8_t gainSmoothing { 0 };
    constexpr unsigned powerTableSizeExponent { 11 };
    constexpr int maxFilePromises { maxVoices };
    constexpr int allSoundOffCC { 120 };
    constexpr int resetCC { 121 };
    constexpr int allNotesOffCC { 123 };
    constexpr int omniOffCC { 124 };
    constexpr int omniOnCC { 125 };
    constexpr int centPerSemitone { 100 };
    constexpr float virtuallyZero { 0.001f };
    constexpr float fastReleaseDuration { 0.01f };
    constexpr char defineCharacter { '$' };
    constexpr Oversampling defaultOversamplingFactor { Oversampling::x1 };
    constexpr float A440 { 440.0 };
    constexpr size_t powerHistoryLength { 16 };
    constexpr float filteredEnvelopeCutoff { 5 };
    constexpr uint16_t numCCs { 512 };
    constexpr int maxCurves { 256 };
    constexpr int chunkSize { 1024 };
    constexpr unsigned int defaultAlignment { 16 };
    constexpr int filtersInPool { maxVoices * 2 };
    constexpr int excessFileFrames { 8 };
    constexpr int maxLFOSubs { 8 };
    constexpr int maxLFOSteps { 128 };
    /**
     * @brief The threshold for age stealing.
     *        In percentage of the voice's max age.
     */
    constexpr float stealingAgeCoeff { 0.5f };
    /**
     * @brief The threshold for envelope stealing.
     *        In percentage of the sum of all envelopes.
     */
    constexpr float stealingEnvelopeCoeff { 0.5f };
    constexpr int filtersPerVoice { 2 };
    constexpr int eqsPerVoice { 3 };
    constexpr int oscillatorsPerVoice { 9 };
    constexpr float uniformNoiseBounds { 0.25f };
    constexpr float noiseVariance { 0.25f };
    /**
       Minimum interval in frames between recomputations of coefficients of the
       modulated filter. The lower, the more CPU resources are consumed.
    */
    constexpr int filterControlInterval { 16 };
    /**
       Default metadata for MIDIName documents
     */
    const absl::string_view midnamManufacturer { "The Sfizz authors" };
    const absl::string_view midnamModel { "Sfizz" };
    /**
       Limit of how many "fxN" buses are accepted (in SFZv2, maximum is 4)
     */
    constexpr int maxEffectBuses { 256 };
    // Wavetable constants; amplitude values are matched to reference
    static constexpr unsigned tableSize = 1024;
    static constexpr double amplitudeSine = 0.625;
    static constexpr double amplitudeTriangle = 0.625;
    static constexpr double amplitudeSaw = 0.515;
    static constexpr double amplitudeSquare = 0.515;
    /**
       Background file loading
     */
    static constexpr int backgroundLoaderPthreadPriority = 50; // expressed in %
    /**
       @brief Ratio to target under which smoothing is considered as completed
     */
    static constexpr float smoothingShortcutThreshold = 5e-3;
} // namespace config

} // namespace sfz
