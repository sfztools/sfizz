#pragma once

namespace sfz
{

namespace config
{
    inline constexpr double defaultSampleRate { 48000 };
    inline constexpr int defaultSamplesPerBlock { 1024 };
    inline constexpr int preloadSize { 32768 };
    inline constexpr int numChannels { 2 };
    inline constexpr int numVoices { 64 };
    inline constexpr int numLoadingThreads { 4 };
    inline constexpr int centPerSemitone { 100 };
    inline constexpr float virtuallyZero { 0.00005f };
    inline constexpr double fastReleaseDuration { 0.01 };
    inline constexpr char defineCharacter { '$' };
    inline constexpr int oversamplingFactor { 2 };
} // namespace config

} // namespace sfz

namespace config
{
    inline constexpr unsigned int defaultAlignment { 16 };
    enum class VectorOperations { standard, sse, neon };
    inline constexpr VectorOperations vectorOperation { VectorOperations::standard };
} // namespace config