// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <array>
#include <bitset>
#include "CCMap.h"
#include "Range.h"

namespace sfz
{
/**
 * @brief Holds the current "MIDI state", meaning the known state of all CCs
 * currently, as well as the note velocities that triggered the currently
 * pressed notes.
 *
 */
class MidiState
{
public:
    MidiState();

    /**
     * @brief Update the state after a note on event
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOnEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Update the state after a note off event
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOffEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Set all notes off
     *
     * @param delay
     */
    void allNotesOff(int delay) noexcept;

    /**
     * @brief Get the number of active notes
     */
    int getActiveNotes() const noexcept { return activeNotes; }

    /**
     * @brief Get the note duration since note on
     *
     * @param noteNumber
     * @param delay
     * @return float
     */
    float getNoteDuration(int noteNumber, int delay = 0) const;

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
     * @brief Get the note on velocity for a given note
     *
     * @param noteNumber
     * @return float
     */
    float getNoteVelocity(int noteNumber) const noexcept;

    /**
     * @brief Get the velocity override value (sw_vel in SFZ)
     *
     * @return float
     */
    float getVelocityOverride() const noexcept;

    /**
     * @brief Register a pitch bend event
     *
     * @param pitchBendValue
     */
    void pitchBendEvent(int delay, float pitchBendValue) noexcept;

    /**
     * @brief Register a pitch bend event on a specific MIDI channel.
     * Out-of-range channels are silently ignored. The single-arg
     * overload forwards to this with channel = masterChannel.
     */
    void pitchBendEvent(int delay, int channel, float pitchBendValue) noexcept;

    /**
     * @brief Get the pitch bend status

     * @return int
     */
    float getPitchBend() const noexcept;

    /**
     * @brief Get the pitch bend status for a specific MIDI channel.
     * Out-of-range channels return 0.0f. Used by per-voice consumers
     * (Voice etc.) to read modulation events scoped to the voice's
     * originating channel; master-channel reads via the no-arg overload
     * are unchanged.
     */
    float getPitchBend(int channel) const noexcept;

    /**
     * @brief Register a channel aftertouch event
     *
     * @param aftertouch
     */
    void channelAftertouchEvent(int delay, float aftertouch) noexcept;

    /**
     * @brief Register a channel aftertouch event on a specific MIDI
     * channel. Out-of-range channels are silently ignored.
     */
    void channelAftertouchEvent(int delay, int channel, float aftertouch) noexcept;

    /**
     * @brief Register a channel aftertouch event
     *
     * @param aftertouch
     */
    void polyAftertouchEvent(int delay, int noteNumber, float aftertouch) noexcept;

    /**
     * @brief Register a polyphonic aftertouch event on a specific MIDI
     * channel. Out-of-range channels or notes are silently ignored.
     */
    void polyAftertouchEvent(int delay, int channel, int noteNumber, float aftertouch) noexcept;

    /**
     * @brief Get the channel aftertouch status

     * @return int
     */
    float getChannelAftertouch() const noexcept;

    /**
     * @brief Get the channel aftertouch status for a specific MIDI
     * channel. Out-of-range channels return 0.0f.
     */
    float getChannelAftertouch(int channel) const noexcept;

    /**
     * @brief Get the polyphonic aftertouch status

     * @return int
     */
    float getPolyAftertouch(int noteNumber) const noexcept;

    /**
     * @brief Get the polyphonic aftertouch status for a specific MIDI
     * channel and note. Out-of-range channels or notes return 0.0f.
     */
    float getPolyAftertouch(int channel, int noteNumber) const noexcept;

    /**
     * @brief Get the current midi program
     *
     * @return int
     */
    int getProgram() const noexcept;
    /**
     * @brief Register a program change event
     *
     * @param delay
     * @param program
     */
    void programChangeEvent(int delay, int program) noexcept;

    /**
     * @brief Register a CC event
     *
     * @param ccNumber
     * @param ccValue
     */
    void ccEvent(int delay, int ccNumber, float ccValue) noexcept;

    /**
     * @brief Register a CC event on a specific MIDI channel.
     * Out-of-range channels are silently ignored.
     */
    void ccEvent(int delay, int channel, int ccNumber, float ccValue) noexcept;

    /**
     * @brief Advances the internal clock of a given amount of samples.
     * You should call this at each callback. This will flush the events
     * in the midistate memory by calling flushEvents().
     *
     * @param numSamples the number of samples of clock advance
     */
    void advanceTime(int numSamples) noexcept;

    /**
     * @brief Returns current internal sample clock
     *
     */
    unsigned getInternalClock() const noexcept { return internalClock; }

    /**
     * @brief Flush events in all states, keeping only the last one as the "base" state
     *
     */
    void flushEvents() noexcept;

    /**
     * @brief Check if a note is currently depressed
     *
     * @param noteNumber
     * @return true
     * @return false
     */
    bool isNotePressed(int noteNumber) const noexcept { return noteStates[noteNumber]; }

    /**
     * @brief Get the last CC value for CC number
     *
     * @param ccNumber
     * @return float
     */
    float getCCValue(int ccNumber) const noexcept;

    /**
     * @brief Get the last CC value for CC number on a specific MIDI
     * channel. Out-of-range channels return 0.0f.
     */
    float getCCValue(int channel, int ccNumber) const noexcept;

    /**
     * @brief Get the CC value for CC number
     *
     * @param ccNumber
     * @param delay
     * @return float
     */
    float getCCValueAt(int ccNumber, int delay) const noexcept;

    /**
     * @brief Get the CC value for CC number on a specific MIDI channel
     * at a given delay. Out-of-range channels return 0.0f.
     */
    float getCCValueAt(int channel, int ccNumber, int delay) const noexcept;

    /**
     * @brief Reset the midi note states
     *
     */
    void resetNoteStates() noexcept;

    const EventVector& getCCEvents(int ccIdx) const noexcept;
    const EventVector& getCCEvents(int channel, int ccIdx) const noexcept;
    const EventVector& getPolyAftertouchEvents(int noteNumber) const noexcept;
    const EventVector& getPolyAftertouchEvents(int channel, int noteNumber) const noexcept;
    const EventVector& getPitchEvents() const noexcept;
    const EventVector& getPitchEvents(int channel) const noexcept;
    /**
     * @brief Return the pitch-bend events for the given channel, with no
     *        master fallback. The vector may be empty if that channel has
     *        never received its own bend events. Use this when combining
     *        member and master bend contributions separately for MPE.
     */
    const EventVector& getPitchEventsRaw(int channel) const noexcept;

    /**
     * @brief Return the latest pitch bend value for the given channel, with
     *        no master fallback. Returns 0 if the channel has never received
     *        its own bend. Pair with getMPEBendRangeForChannel to compute a
     *        per-channel bend contribution.
     */
    float getPitchBendRaw(int channel) const noexcept;
    const EventVector& getChannelAftertouchEvents() const noexcept;
    const EventVector& getChannelAftertouchEvents(int channel) const noexcept;
    /**
     * @brief Reset the midi event states (CC, AT, and pitch bend)
     *
     */
    void resetEventStates() noexcept;

    /**
     * @brief Configure the MPE pitch bend ranges used when computing the
     *        bend amount applied to a voice. Master and member channels
     *        carry independent ranges per MPE 1.0; SFZ regions only have
     *        a single bend_up / bend_down pair, so member channels can't
     *        rely on the region opcodes and need this synth-level setting.
     */
    void setMPEPitchBendRange(float masterSemitones, float perNoteSemitones) noexcept;

    /**
     * @brief Return the bend range (in semitones) the given channel should
     *        use to scale a normalized [-1, +1] pitch bend value. Master
     *        channel returns the master range, member channels return the
     *        per-note range.
     */
    float getMPEBendRangeForChannel(int channel) const noexcept;

private:

    /**
     * @brief Insert events in a sorted event vector.
     *
     * @param events
     * @param delay
     * @param value
     */
    void insertEventInVector(EventVector& events, int delay, float value);

    int activeNotes { 0 };

    /**
     * @brief Stores the note on times.
     *
     */
    MidiNoteArray<unsigned> noteOnTimes { {} };

    /**
     * @brief Stores the note off times.
     *
     */

    MidiNoteArray<unsigned> noteOffTimes { {} };

    /**
     * @brief Store the note states
     *
     */
    std::bitset<128> noteStates;

    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
    MidiNoteArray<float> lastNoteVelocities;

    /**
     * @brief Velocity override value (sw_vel in SFZ)
     */
    float velocityOverride;

    /**
     * @brief Last note played
     */
    int lastNotePlayed { -1 };

    /**
     * @brief Per-channel event state. Holds the pitch/CC/aftertouch event
     * vectors for one MIDI channel. Introduced so MPE-aware callers can
     * route events to a specific member channel without colliding with
     * other channels' modulation. M1 wires only the master channel; M3
     * will add channel-aware public API methods that target channels
     * 1..15. Until then, all events resolve to channelStates[masterChannel]
     * and behavior is byte-for-byte identical to the pre-refactor code.
     */
    struct ChannelState {
        std::array<EventVector, config::numCCs> ccEvents;
        std::array<EventVector, 128> polyAftertouchEvents;
        EventVector pitchEvents;
        EventVector channelAftertouchEvents;
    };

    /**
     * @brief Per-channel pitch/CC/aftertouch state. Indexed 0..15 to match
     * MIDI channels 1..16 (0-indexed). The master channel for non-MPE
     * input is index 0; MPE member channels occupy 1..15 (or 0..14 with
     * channel 16 as master, depending on zone configuration — currently
     * fixed at master=0 pending M3).
     */
    static constexpr int masterChannel = 0;
    std::array<ChannelState, 16> channelStates;

    // MPE 1.0 defaults: master = 2 st, per-note = 48 st.
    float mpeMasterPitchBendRange_ { 2.0f };
    float mpePerNotePitchBendRange_ { 48.0f };

    /**
     * @brief Null event
     *
     */
    const EventVector nullEvent { { 0, 0.0f } };

    /**
     * @brief Current midi program
     */
    int currentProgram { 0 };

    float sampleRate { config::defaultSampleRate };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    float alternate { 0.0f };
    unsigned internalClock { 0 };
    fast_real_distribution<float> unipolarDist { 0.0f, 1.0f };
    fast_real_distribution<float> bipolarDist { -1.0f, 1.0f };
};
}
