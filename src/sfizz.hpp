// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
  @file
  @brief sfizz public C++ API
*/

#include <string>
#include <utility>
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
/**
 * @brief Main class
 */
class SFIZZ_EXPORTED_API Sfizz
{
public:
    /**
     * @brief Construct a new Sfizz object. The synth by default is set at 48 kHz
     * and a block size of 1024. You should change these values if they are not
     * suited to your application.
     *
     */
    Sfizz();
    ~Sfizz();

    /**
     * @brief Empties the current regions and load a new SFZ file into the synth.
     *
     * This function will disable all callbacks so it is safe to call from a
     * UI thread for example, although it may generate a click. However it is
     * not reentrant, so you should not call it from concurrent threads.
     *
     * @param path The path to the file to load, as string.
     *
     * @return @false if the file was not found or no regions were loaded,
     *         @true otherwise.
     */
    bool loadSfzFile(const std::string& path);

    /**
     * @brief Return the current number of regions loaded.
     */
    int getNumRegions() const noexcept;

    /**
     * @brief Return the current number of groups loaded.
     */
    int getNumGroups() const noexcept;

    /**
     * @brief Return the current number of masters loaded.
     */
    int getNumMasters() const noexcept;

    /**
     * @brief Return the current number of curves loaded.
     */
    int getNumCurves() const noexcept;

    /**
     * @brief Return a list of unsupported opcodes, if any.
     */
    const std::vector<std::string>& getUnknownOpcodes() const noexcept;

    /**
     * @brief Return the number of preloaded samples in the synth.
     */
    size_t getNumPreloadedSamples() const noexcept;

    /**
     * @brief Set the maximum size of the blocks for the callback. The actual
     * size can be lower in each callback but should not be larger
     * than this value.
     *
     * @param samplesPerBlock The number of samples per block.
     */
    void setSamplesPerBlock(int samplesPerBlock) noexcept;

    /**
     * @brief Set the sample rate. If you do not call it it is initialized
     * to sfz::config::defaultSampleRate.
     *
     * @param sampleRate The sample rate.
     */
    void setSampleRate(float sampleRate) noexcept;

    /**
     * @brief Return the current value for the volume, in dB.
     */
    float getVolume() const noexcept;

     /**
     * @brief Set the value for the volume. This value will be
     * clamped within sfz::default::volumeRange.
     *
     * @param volume The new volume.
     */
    void setVolume(float volume) noexcept;

    /**
     * @brief Send a note on event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to renderBlock().
     * @param noteNumber the midi note number.
     * @param velocity the midi note velocity.
     */
    void noteOn(int delay, int noteNumber, uint8_t velocity) noexcept;

    /**
     * @brief Send a note off event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to renderBlock().
     * @param noteNumber the midi note number.
     * @param velocity the midi note velocity.
     */
    void noteOff(int delay, int noteNumber, uint8_t velocity) noexcept;

    /**
     * @brief Send a CC event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param ccNumber the cc number.
     * @param ccValue the cc value.
     */
    void cc(int delay, int ccNumber, uint8_t ccValue) noexcept;

    /**
     * @brief Send a pitch bend event to the synth
     *
     * @param delay the delay at which the event occurs; this should be lower
     *              than the size of the block in the next call to
     *              renderBlock().
     * @param pitch the pitch value centered between -8192 and 8192.
     */
    void pitchWheel(int delay, int pitch) noexcept;

    /**
     * @brief Send a aftertouch event to the synth. (CURRENTLY UNIMPLEMENTED)
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param aftertouch the aftertouch value.
     */
    void aftertouch(int delay, uint8_t aftertouch) noexcept;

    /**
     * @brief Send a tempo event to the synth. (CURRENTLY UNIMPLEMENTED)
     *
     * @param delay the delay at which the event occurs; this should be lower than the size of
     *              the block in the next call to renderBlock().
     * @param secondsPerQuarter the new period of the quarter note.
     */
    void tempo(int delay, float secondsPerQuarter) noexcept;

    /**
     * @brief Render an block of audio data in the buffer. This call will reset
     * the synth in its waiting state for the next batch of events. The buffers must
     * be float[numSamples][numOutputs * 2].
     *
     * @param buffers the buffers to write the next block into.
     * @param numFrames the number of stereo frames in the block.
     * @param numOutputs the number of stereo outputs.
     */
    void renderBlock(float** buffers, size_t numFrames, int numOutputs = 1) noexcept;

    /**
     * @brief Return the number of active voices.
     */
    int getNumActiveVoices() const noexcept;

    /**
     * @brief Return the total number of voices in the synth (the polyphony).
     */
    int getNumVoices() const noexcept;

    /**
     * @brief Change the number of voices (the polyphony).
     *
     * @param numVoices The number of voices.
     */
    void setNumVoices(int numVoices) noexcept;

    /**
     * @brief Set the oversampling factor to a new value. This will disable all callbacks
     * kill all the voices, and trigger a reloading of every file in the FilePool under
     * the new oversampling.
     *
     * @param factor The oversampling factor.
     *
     * @return @true if the factor did indeed change, @false otherwise.
     */
    bool setOversamplingFactor(int factor) noexcept;

    /**
     * @brief Return the current oversampling factor.
     */
    int getOversamplingFactor() const noexcept;

    /**
     * @brief Set the preloaded file size. This will disable the callback.
     *
     * @param preloadSize  The preload size.
     */
    void setPreloadSize(uint32_t preloadSize) noexcept;

    /**
     * @brief Return the current preloaded file size.
     */
    uint32_t getPreloadSize() const noexcept;

    /**
     * @brief Return the number of allocated buffers.
     */
    int getAllocatedBuffers() const noexcept;

    /**
     * @brief Return the number of bytes allocated through the buffers.
     */
    int getAllocatedBytes() const noexcept;

    /**
     * @brief Enable freewheeling on the synth. This will wait for background
     * loaded files to finish loading before each render callback to ensure that
     * there will be no dropouts.
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
     * @return @true if any included files (including the root file) have
     *         been modified since the sfz file was loaded, @false otherwise.
     */
    bool shouldReloadFile();

    /**
     * @brief Enable logging of timings to sidecar CSV files. This can produce
     * many outputs so use with caution.
     *
     * @param prefix the file prefix to use for logging.
     */
    void enableLogging() noexcept;

    /**
     * @brief Enable logging of timings to sidecar CSV files. This can produce
     * many outputs so use with caution.
     *
     * @param prefix the file prefix to use for logging.
     */
    void enableLogging(const std::string& prefix) noexcept;

    /**
     * @brief Set the logging prefix.
     *
     * @param prefix
     */
    void setLoggingPrefix(const std::string& prefix) noexcept;

    /**
     * @brief
     *
     */
    void disableLogging() noexcept;

    /**
     * @brief Shuts down the current processing, clear buffers and reset the voices.
     */
    void allSoundOff() noexcept;

    /**
     * @brief Add external definitions prior to loading;
     * Note that these do not get reset by loading or resetting the synth.
     * You need to call clearExternalDefintions() to erase them.
     *
     * @param id
     * @param value
     */
    void addExternalDefinition(const std::string& id, const std::string& value);

    /**
     * @brief Clears external definitions for the next file loading.
     *
     */
    void clearExternalDefinitions();

    /**
     * @brief Get the note labels, if any
     * @version 0.4.0-dev
     *
     */
    const std::vector<std::pair<uint8_t, std::string>>& getNoteLabels() const noexcept;
    /**
     * @brief Get the CC labels, if any
     * @version 0.4.0-dev
     *
     */
    const std::vector<std::pair<uint16_t, std::string>>& getCCLabels() const noexcept;
private:
    std::unique_ptr<sfz::Synth> synth;
};
}
