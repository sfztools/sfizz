#pragma once

namespace sfz
{

namespace config
{
    constexpr float defaultSampleRate { 48000 };
    constexpr int defaultSamplesPerBlock { 1024 };
    constexpr int preloadSize { 8192 };
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

namespace SIMDConfig
{
    constexpr unsigned int defaultAlignment { 16 };
    constexpr bool writeInterleaved { true };
    constexpr bool readInterleaved { true };
    constexpr bool fill { true };
    constexpr bool gain { false };
    constexpr bool mathfuns { false };
    constexpr bool loopingSFZIndex { true };
    constexpr bool linearRamp { false };
    constexpr bool multiplicativeRamp { true };
    constexpr bool add { false };
#if USE_SIMD
    constexpr bool useSIMD { true };
#else
    constexpr bool useSIMD { false };
#endif
}