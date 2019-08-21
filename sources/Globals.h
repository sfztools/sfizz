#pragma once

namespace sfz
{

namespace config
{
    constexpr double defaultSampleRate { 48000 };
    constexpr int defaultSamplesPerBlock { 1024 };
    constexpr int preloadSize { 32768 };
    constexpr int numChannels { 2 };
    constexpr int numVoices { 64 };
    constexpr int numLoadingThreads { 4 };
    constexpr int centPerSemitone { 100 };
    constexpr float virtuallyZero { 0.00005f };
    constexpr double fastReleaseDuration { 0.01 };
    constexpr char defineCharacter { '$' };
    constexpr int oversamplingFactor { 2 };
} // namespace config

} // namespace sfz

namespace SIMDConfig
{
    constexpr unsigned int defaultAlignment { 16 };
    constexpr bool writeInterleaved { true };
    constexpr bool readInterleaved { true };
    constexpr bool fill { false };
    constexpr bool gain { false };
    constexpr bool mathfuns { false };
#if USE_SIMD
    constexpr bool useSIMD { true };
#else
    constexpr bool useSIMD { false };
#endif
}