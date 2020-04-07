// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Resources.h"
#include "Parser.h"
#include "Voice.h"
#include "Region.h"
#include "Effects.h"
#include "Curve.h"
#include "LeakDetector.h"
#include "MidiState.h"
#include "AudioSpan.h"
#include "parser/Parser.h"
#include "absl/types/span.h"
#include <absl/types/optional.h>
#include <random>
#include <mutex>
#include <set>
#include <string_view>
#include <vector>

namespace sfz {
/**
 * @brief This class is the core of the sfizz library. In C++ it is the main point
 * of entry and in C the interface basically maps the functions of the class into
 * C bindings.
 *
 * The JACK client provides an example of how you can use this class as an entry
 * point for your own projects. Just include this header and compile against the
 * static library. If you wish to use the shared library you should rather use the
 * C bindings.
 *
 * This class derives from the Parser and provides a specific set of callbacks; see
 * the Parser documentation for more precisions.
 *
 * The Synth object contains:
 * - A set of SFZ Regions that get filled up upon parsing
 * - A set of Voices that play the sounds of the regions when triggered.
 * - Some singleton resources, particularly the midiState which contains the current
 *      midi status (note is on or off, last note velocity, current CC values, ...)
 *      as well as a FilePool that preloads and give access to files.
 *
 * The synth is callback based, in the sense that it renders audio block by block
 * using the renderBlock() function. Between each call to renderBlock() you have to
 * send the relevent events for the block in the form of MIDI events: noteOn(),
 * noteOff(), cc(). You can also send pitchBend(), aftertouch() and bpm()
 * events -- but as of 2019 they are not handled.
 *
 * All events have a delay information, which must be less than the size of the
 * next call to renderBlock() in units of frames or samples. For example, if you
 * will call to render a block of 256 samples, all the events you send to the
 * synth should have a delay parameter strictly lower than 256. Events beyond 256
 * may be completely ignored by the synth as the incoming event buffer is cleared
 * during the renderBlock() call. You SHOULD also feed the midi events in the correct
 * order.
 *
 * The jack_client.cpp file contains examples of the most classical usage of the
 * synth and can be used as a reference.
 */
class Synth final : public Parser::Listener {
public:
    /**
     * @brief Construct a new Synth object with no voices. If you want sound
     * you will need to call setNumVoices() before playing.
     *
     */
    Synth();
    /**
     * @brief Destructor
     */
    ~Synth();
    /**
     * @brief Construct a new Synth object with a specified number of voices.
     *
     * @param numVoices
     */
    Synth(int numVoices);
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
    bool loadSfzFile(const fs::path& file);
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
    /**
     * @brief Export a MIDI Name document describing the loaded instrument
     */
    std::string exportMidnam(absl::string_view model = {}) const;
    /**
     * @brief Get a raw view into a specific region. This is mostly used
     * for testing.
     *
     * @param idx
     * @return const Region*
     */
    const Region* getRegionView(int idx) const noexcept;
    /**
     * @brief Get a raw view into a specific voice. This is mostly used
     * for testing.
     *
     * @param idx
     * @return const Region*
     */
    const Voice* getVoiceView(int idx) const noexcept;
    /**
     * @brief Get a raw view into a specific voice. This is mostly used
     * for testing.
     *
     * @param idx
     * @return const Region*
     */
    const EffectBus* getEffectBusView(int idx) const noexcept;
    /**
     * @brief Get a list of unknown opcodes. The lifetime of the
     * string views in the code are linked to the currently loaded
     * sfz file.
     *
     * TODO: change this to strings we don't really care about performance
     * here and this hurts the C interface.
     *
     * @return std::set<absl::string_view>
     */
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
     * the synth in its waiting state for the next batch of events. The size of
     * the block is integrated in the AudioSpan object. You can build an
     * AudioSpan implicitely from a large number of source objects; check the
     * AudioSpan reference for more precision.
     *
     * @param buffer the buffer to write the next block into; this should be a
     * stereo buffer.
     */
    void renderBlock(AudioSpan<float> buffer) noexcept;

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
     * @brief Trigger a garbage collection, which removes the samples that are
     * loaded by the FilePool after being requested by the voices. This does
     * not concern the preloaded samples, only the samples loaded to be played
     * fully. This function is run regularly in a background thread so normally
     * you should not need to call it explicitely.
     *
     */
    void garbageCollect() noexcept;

    /**
     * @brief Set the oversampling factor to a new value. This will disable all callbacks
     * kill all the voices, and trigger a reloading of every file in the FilePool under
     * the new oversampling.
     *
     * @param factor
     */
    void setOversamplingFactor(Oversampling factor) noexcept;

    /**
     * @brief get the current oversampling factor
     *
     * @return Oversampling
     */
    Oversampling getOversamplingFactor() const noexcept;

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
    int getAllocatedBuffers() const noexcept { return Buffer<float>::counter().getNumBuffers(); }

    /**
     * @brief      Gets the number of bytes allocated through the buffers
     *
     * @return     The allocated bytes.
     */
    int getAllocatedBytes() const noexcept { return Buffer<float>::counter().getTotalBytes(); }

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

    const MidiState& getMidiState() const noexcept { return resources.midiState; }

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
    void enableLogging(absl::string_view prefix = "") noexcept;
    /**
     * @brief Enable logging of timings to sidecar CSV files. This can produce
     * many outputs so use with caution.
     *
     */
    void setLoggingPrefix(absl::string_view prefix) noexcept;
    /**
     * @brief Disable logging;
     *
     */
    void disableLogging() noexcept;

    /**
     * @brief Shuts down the current processing, clear buffers and reset the voices.
     *
     */
    void allSoundOff() noexcept;

    /**
     * @brief      Get the parser.
     *
     * @return     A reference to the parser.
     */
    Parser& getParser() noexcept { return parser; }
    /**
     * @brief      Get the parser.
     *
     * @return     A reference to the parser.
     */
    const Parser& getParser() const noexcept { return parser; }

protected:
    /**
     * @brief The parser callback; this is called by the parent object each time
     * a new region, group, master, global, curve or control set of opcodes
     * appears in the parser
     *
     * @param header the header for the set of opcodes
     * @param members the opcode members
     */
    void onParseFullBlock(const std::string& header, const std::vector<Opcode>& members) override;

    /**
     * @brief The parser callback when an error occurs.
     */
    void onParseError(const SourceRange& range, const std::string& message) override;

    /**
     * @brief The parser callback when a warning occurs.
     */
    void onParseWarning(const SourceRange& range, const std::string& message) override;

private:
    /**
     * @brief change the group maximum polyphony
     *
     * @param groupIdx the group index
     * @param polyphone the max polyphony
     */
    void setGroupPolyphony(unsigned groupIdx, unsigned polyphony) noexcept;
    std::vector<unsigned> groupMaxPolyphony { config::maxVoices };

    /**
     * @brief Reset all CCs; to be used on CC 121
     *
     * @param delay the delay for the controller reset
     *
     */
    void resetAllControllers(int delay) noexcept;

    int numGroups { 0 };
    int numMasters { 0 };

    /**
     * @brief Remove all regions, resets all voices and clears everything
     * to bring back the synth in its original state.
     *
     */
    void clear();

    /**
     * @brief Helper function to dispatch <global> opcodes
     *
     * @param members the opcodes of the <global> block
     */
    void handleGlobalOpcodes(const std::vector<Opcode>& members);
    /**
     * @brief Helper function to dispatch <group> opcodes
     *
     * @param members the opcodes of the <group> block
     */
    void handleGroupOpcodes(const std::vector<Opcode>& members, const std::vector<Opcode>& masterMembers);
    /**
     * @brief Helper function to dispatch <control> opcodes
     *
     * @param members the opcodes of the <control> block
     */
    void handleControlOpcodes(const std::vector<Opcode>& members);
    /**
     * @brief Helper function to dispatch <effect> opcodes
     *
     * @param members the opcodes of the <effect> block
     */
    void handleEffectOpcodes(const std::vector<Opcode>& members);
    /**
     * @brief Helper function to merge all the currently active opcodes
     * as set by the successive callbacks and create a new region to store
     * in the synth.
     *
     * @param regionOpcodes the opcodes that are specific to the region
     */
    void buildRegion(const std::vector<Opcode>& regionOpcodes);
    /**
     * @brief Resets and possibly changes the number of voices (polyphony) in
     * the synth.
     *
     * @param numVoices
     */
    void resetVoices(int numVoices);

    fs::file_time_type checkModificationTime();

    void noteOnDispatch(int delay, int noteNumber, float velocity) noexcept;
    void noteOffDispatch(int delay, int noteNumber, float velocity) noexcept;

    // Opcode memory; these are used to build regions, as a new region
    // will integrate opcodes from the group, master and global block
    std::vector<Opcode> globalOpcodes;
    std::vector<Opcode> masterOpcodes;
    std::vector<Opcode> groupOpcodes;

    /**
     * @brief Find a voice that is not currently playing
     *
     * @return Voice*
     */
    Voice* findFreeVoice() noexcept;
    // Names for the cc as set by the label_cc opcode
    std::vector<CCNamePair> ccNames;
    // Default active switch if multiple keyswitchable regions are present
    absl::optional<uint8_t> defaultSwitch;
    std::vector<std::string> unknownOpcodes;
    using RegionPtrVector = std::vector<Region*>;
    using VoicePtrVector = std::vector<Voice*>;
    std::vector<std::unique_ptr<Region>> regions;
    std::vector<std::unique_ptr<Voice>> voices;
    // Views to speed up iteration over the regions and voices when events
    // occur in the audio callback
    VoicePtrVector voiceViewArray;
    std::array<RegionPtrVector, 128> noteActivationLists;
    std::array<RegionPtrVector, config::numCCs> ccActivationLists;

    // Effect factory and buses
    EffectFactory effectFactory;
    typedef std::unique_ptr<EffectBus> EffectBusPtr;
    std::vector<EffectBusPtr> effectBuses; // 0 is "main", 1-N are "fx1"-"fxN"

    // Curves
    CurveSet curves;

    int samplesPerBlock { config::defaultSamplesPerBlock };
    float sampleRate { config::defaultSampleRate };
    float volume { Default::globalVolume };
    int numVoices { config::numVoices };
    Oversampling oversamplingFactor { config::defaultOversamplingFactor };

    // Distribution used to generate random value for the *rand opcodes
    std::uniform_real_distribution<float> randNoteDistribution { 0, 1 };
    unsigned fileTicket { 1 };

    std::mutex callbackGuard;
    bool freeWheeling { false };

    // Singletons passed as references to the voices
    Resources resources;

    // Control opcodes
    std::string defaultPath { "" };
    int noteOffset { 0 };
    int octaveOffset { 0 };

    Duration dispatchDuration { 0 };

    Parser parser;
    fs::file_time_type modificationTime { };

    LEAK_DETECTOR(Synth);
};

}
