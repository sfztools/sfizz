// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <string>
#include <vector>
#include <memory>

#if defined SFIZZ_EXPORT_SYMBOLS
  #if defined _WIN32
    #define SFIZZ_EXPORTED_API __declspec(dllexport)
  #else
    #define SFIZZ_EXPORTED_API __attribute__ ((visibility ("default")))
  #endif
#else
  #define SFIZZ_EXPORTED_API
#endif

namespace sfz
{
class Synth;
class SFIZZ_EXPORTED_API Sfizz
{
public:
    Sfizz();
    ~Sfizz();
    /**
     * @brief Empties the current regions and load a new SFZ file into the synth.
     *
     * This function will disable all callbacks so it is safe to call from a
     * UI thread for example, although it may generate a click. However it is
     * not reentrant, so you should not call it from concurrent threads.
     *
     * @param file
     * @return true
     * @return false if the file was not found or no regions were loaded.
     */
    bool loadSfzFile(const std::string& path);
    /**
     * @brief Get the current number of regions loaded
     *
     * @return int
     */
    int getNumRegions() const noexcept;
    /**
     * @brief Get the current number of groups loaded
     *
     * @return int
     */
    int getNumGroups() const noexcept;
    /**
     * @brief Get the current number of masters loaded
     *
     * @return int
     */
    int getNumMasters() const noexcept;
    /**
     * @brief Get the current number of curves loaded
     *
     * @return int
     */
    int getNumCurves() const noexcept;
    const std::vector<std::string>& getUnknownOpcodes() const noexcept;
    /**
     * @brief Get the number of preloaded samples in the synth
     *
     * @return size_t
     */
    size_t getNumPreloadedSamples() const noexcept;
    /**
     * @brief Set the maximum size of the blocks for the callback. The actual
     * size can be lower in each callback but should not be larger
     * than this value.
     *
     * @param samplesPerBlock
     */
    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    /**
     * @brief Set the sample rate. If you do not call it it is initialized
     * to sfz::config::defaultSampleRate.
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate) noexcept;
    /**
     * @brief Get the current value for the volume, in dB.
     *
     * @return float
     */
    float getVolume() const noexcept;
    /**
     * @brief Set the value for the volume. This value will be
     * clamped within sfz::default::volumeRange.
     *
     * @param volume
     */
    void setVolume(float volume) noexcept;

    /**
     * @brief Send a note on event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to renderBlock().
     * @param noteNumber the midi note number
     * @param velocity the midi note velocity
     */
    void noteOn(int delay, int noteNumber, uint8_t velocity) noexcept;
    /**
     * @brief Send a note off event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to renderBlock().
     * @param noteNumber the midi note number
     * @param velocity the midi note velocity
     */
    void noteOff(int delay, int noteNumber, uint8_t velocity) noexcept;
    /**
     * @brief Send a CC event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param ccNumber the cc number
     * @param ccValue the cc value
     */
    void cc(int delay, int ccNumber, uint8_t ccValue) noexcept;
    /**
     * @brief Send a pitch bend event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to
     *              renderBlock().
     * @param pitch the pitch value centered between -8192 and 8192
     */
    void pitchWheel(int delay, int pitch) noexcept;
    /**
     * @brief Send a aftertouch event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param aftertouch the aftertouch value
     */
    void aftertouch(int delay, uint8_t aftertouch) noexcept;
    /**
     * @brief Send a tempo event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param secondsPerQuarter the new period of the quarter note
     */
    void tempo(int delay, float secondsPerQuarter) noexcept;
    /**
     * @brief Render an block of audio data in the buffer. This call will reset
     * the synth in its waiting state for the next batch of events. The buffers must
     * be float[numSamples][numOutputs * 2].
     *
     * @param buffers the buffers to write the next block into
     * @param numSamples the number of stereo frames in the block
     * @param numOutputs the number of stereo outputs
     */
    void renderBlock(float** buffers, size_t numFrames, int numOutputs = 1) noexcept;

    /**
     * @brief Get the number of active voices
     *
     * @return int
     */
    int getNumActiveVoices() const noexcept;
    /**
     * @brief Get the total number of voices in the synth (the polyphony)
     *
     * @return int
     */
    int getNumVoices() const noexcept;
    /**
     * @brief Change the number of voices (the polyphony)
     *
     * @param numVoices
     */
    void setNumVoices(int numVoices) noexcept;

        /**
     * @brief Set the oversampling factor to a new value. This will disable all callbacks
     * kill all the voices, and trigger a reloading of every file in the FilePool under
     * the new oversampling.
     *
     * @param factor
     * @return true if the factor did indeed change
     */
    bool setOversamplingFactor(int factor) noexcept;

    /**
     * @brief get the current oversampling factor
     *
     * @return Oversampling
     */
    int getOversamplingFactor() const noexcept;

    /**
     * @brief Set the preloaded file size. This will disable the callback.
     *
     * @param factor
     */
    void setPreloadSize(uint32_t preloadSize) noexcept;

    /**
     * @brief get the current preloaded file size
     *
     * @return Oversampling
     */
    uint32_t getPreloadSize() const noexcept;

    /**
     * @brief      Gets the number of allocated buffers.
     *
     * @return     The allocated buffers.
     */
    int getAllocatedBuffers() const noexcept;

    /**
     * @brief      Gets the number of bytes allocated through the buffers
     *
     * @return     The allocated bytes.
     */
    int getAllocatedBytes() const noexcept;

    /**
     * @brief Enable freewheeling on the synth. This will wait for background
     * loaded files to finish loading before each render callback to ensure that
     * there will be no dropouts.
     *
     */
    void enableFreeWheeling() noexcept;
    /**
     * @brief Disable freewheeling on the synth. You should disable freewheeling
     * before live use of the plugin otherwise the audio thread will lock.
     *
     */
    void disableFreeWheeling() noexcept;
    /**
     * @brief Check if the SFZ should be reloaded.
     *
     * Depending on the platform this can create file descriptors.
     *
     * @return true if any included files (including the root file) have
     *              been modified since the sfz file was loaded.
     * @return false
     */
    bool shouldReloadFile();
    /**
     * @brief Enable logging of timings to sidecar CSV files. This can produce
     * many outputs so use with caution.
     *
     */
    void enableLogging() noexcept;
    /**
     * @brief Disable logging;
     *
     */
    void disableLogging() noexcept;
private:
    std::unique_ptr<sfz::Synth> synth;
};
}
