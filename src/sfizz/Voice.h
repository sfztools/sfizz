// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "TriggerEvent.h"
#include "Config.h"
#include "ADSREnvelope.h"
#include "HistoricalBuffer.h"
#include "Region.h"
#include "AudioBuffer.h"
#include "Resources.h"
#include "Smoothers.h"
#include "AudioSpan.h"
#include "LeakDetector.h"
#include "OnePoleFilter.h"
#include "PowerFollower.h"
#include "utility/NumericId.h"
#include "absl/types/span.h"
#include <memory>
#include <random>

namespace sfz {
enum InterpolatorModel : int;
class LFO;
/**
 * @brief The SFZ voice are the polyphony holders. They get activated by the synth
 * and tasked to play a given region until the end, stopping on note-offs, off-groups
 * or natural sample decay.
 *
 */
class Voice {
public:
    Voice() = delete;
    /**
     * @brief Construct a new voice with the midistate singleton
     *
     * @param voiceNumber
     * @param midiState
     */
    Voice(int voiceNumber, Resources& resources);

    ~Voice();

    /**
     * @brief Get the unique identifier of this voice in a synth
     */
    NumericId<Voice> getId() const noexcept
    {
        return id;
    }

    enum class State {
        idle,
        playing,
        cleanMeUp,
    };

    class StateListener {
    public:
        virtual void onVoiceStateChanged(NumericId<Voice> /*id*/, State /*state*/) {}
    };

    /**
     * @brief Return true if the voice is to be cleaned up (zombie state)
     */
    bool toBeCleanedUp() const { return state == State::cleanMeUp; }

    /**
     * @brief Sets the listener which is called when the voice state changes.
     */
    void setStateListener(StateListener *l) noexcept { stateListener = l; }

    /**
     * @brief Change the sample rate of the voice. This is used to compute all
     * pitch related transformations so it needs to be propagated from the synth
     * at all times.
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate) noexcept;
    /**
     * @brief Set the expected block size. If the block size is not fixed, set an
     * upper bound. The voice will adapt at each callback to the actual number of
     * samples requested but this function will allocate temporary buffers that are
     * needed for proper functioning.
     *
     * @param samplesPerBlock
     */
    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    /**
     * @brief Get the sample rate of the voice.
     *
     * @return float
     */
    float getSampleRate() const noexcept { return sampleRate; }
    /**
     * @brief Get the expected block size.
     *
     * @return int
     */
    int getSamplesPerBlock() const noexcept { return samplesPerBlock; }

    /**
     * @brief Start playing a region after a short delay for different triggers (note on, off, cc)
     *
     * @param region
     * @param delay
     * @param evebt
     */
    void startVoice(Region* region, int delay, const TriggerEvent& event) noexcept;

    /**
     * @brief Get the sample quality determined by the active region.
     *
     * @return int
     */
    int getCurrentSampleQuality() const noexcept;

    /**
     * @brief Register a note-off event; this may trigger a release.
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void registerNoteOff(int delay, int noteNumber, float velocity) noexcept;
    /**
     * @brief Register a CC event; this may trigger a release. If the voice is playing and its
     * region has CC modifiers, it will use this value to compute the CC envelope to apply to the
     * parameter.
     *
     * @param delay
     * @param ccNumber
     * @param ccValue
     */
    void registerCC(int delay, int ccNumber, float ccValue) noexcept;
    /**
     * @brief Register a pitch wheel event; for now this does nothing
     *
     * @param delay
     * @param pitch
     */
    void registerPitchWheel(int delay, float pitch) noexcept;
    /**
     * @brief Register an aftertouch event; for now this does nothing
     *
     * @param delay
     * @param aftertouch
     */
    void registerAftertouch(int delay, uint8_t aftertouch) noexcept;
    /**
     * @brief Register a tempo event; for now this does nothing
     *
     * @param delay
     * @param pitch
     */
    void registerTempo(int delay, float secondsPerQuarter) noexcept;
    /**
     * @brief Checks if the voice should be offed by another starting in the group specified.
     * This will trigger the release if true.
     *
     * @param delay
     * @param noteNumber
     * @param group
     * @return true
     * @return false
     */
    bool checkOffGroup(const Region* other, int delay, int noteNumber) noexcept;

    /**
     * @brief Render a block of data for this voice into the span
     *
     * @param buffer
     */
    void renderBlock(AudioSpan<float, 2> buffer) noexcept;

    /**
     * @brief Is the voice free?
     *
     * @return true
     * @return false
     */
    bool isFree() const noexcept;
    /**
     * @brief Can the voice be reused (i.e. is it releasing or free)
     *
     * @return true
     * @return false
     */
    bool releasedOrFree() const noexcept;
    /**
     * @brief Get the event that triggered the voice
     *
     * @return int
     */
    const TriggerEvent& getTriggerEvent() const noexcept { return triggerEvent; }

    /**
     * @brief Reset the voice to its initial values
     *
     */
    void reset() noexcept;

    /**
     * @brief Set the next voice in the "sister voice" ring
     * The sister voices are voices that started on the same event.
     * This has to be set by the synth. A voice will remove itself from
     * the ring upon reset.
     *
     * @param voice
     */
    void setNextSisterVoice(Voice* voice) noexcept;

    /**
     * @brief Set the previous voice in the "sister voice" ring
     * The sister voices are voices that started on the same event.
     * This has to be set by the synth. A voice will remove itself from
     * the ring upon reset.
     *
     * @param voice
     */
    void setPreviousSisterVoice(Voice* voice) noexcept;

    /**
     * @brief Get the next sister voice in the ring
     *
     * @return Voice*
     */
    Voice* getNextSisterVoice() const noexcept { return nextSisterVoice; };

    /**
     * @brief Get the previous sister voice in the ring
     *
     * @return Voice*
     */
    Voice* getPreviousSisterVoice() const noexcept { return previousSisterVoice; };

    /**
     * @brief Get the mean squared power of the last rendered block. This is used
     * to determine which voice to steal if there are too many notes flying around.
     *
     * @return float
     */
    float getAveragePower() const noexcept;
    /**
     * @brief Get the position of the voice in the source, in samples
     *
     * @return uint32_t
     */
    uint32_t getSourcePosition() const noexcept;
    /**
     * Returns the region that is currently playing. May be null if the voice is not active!
     *
     * @return
     */
    const Region* getRegion() const noexcept { return region; }
    /**
     * @brief Get the LFO designated by the given index
     *
     * @param index
     */
    LFO* getLFO(size_t index) { return lfos[index].get(); }
    /**
     * @brief Set the max number of filters per voice
     *
     * @param numFilters
     */
    void setMaxFiltersPerVoice(size_t numFilters);
    /**
     * @brief Set the max number of EQs per voice
     *
     * @param numEQs
     */
    void setMaxEQsPerVoice(size_t numEQs);
    /**
     * @brief Set the max number of LFOs per voice
     *
     * @param numLFOs
     */
    void setMaxLFOsPerVoice(size_t numLFOs);
    /**
     * @brief Release the voice after a given delay
     *
     * @param delay
     * @param fastRelease whether to do a normal release or cut the voice abruptly
     */
    void release(int delay) noexcept;

    /**
     * @brief Off the voice (steal). This will respect the off mode of the region
     *      and set the envelopes if necessary.
     *
     * @param delay
     */
    void off(int delay) noexcept;

    /**
     * @brief gets the age of the Voice
     *
     * @return
     */
    int getAge() const noexcept { return age; }

    Duration getLastDataDuration() const noexcept { return dataDuration; }
    Duration getLastAmplitudeDuration() const noexcept { return amplitudeDuration; }
    Duration getLastFilterDuration() const noexcept { return filterDuration; }
    Duration getLastPanningDuration() const noexcept { return panningDuration; }

private:
    /**
     * @brief Fill a span with data from a file source. This is the first step
     * in rendering each block of data.
     *
     * @param buffer
     */
    void fillWithData(AudioSpan<float> buffer) noexcept;
    /**
     * @brief Fill a span with data from a generator source. This is the first step
     * in rendering each block of data.
     *
     * @param buffer
     */
    void fillWithGenerator(AudioSpan<float> buffer) noexcept;

    /**
     * @brief Fill a destination with an interpolated source.
     *
     * @param source the source sample
     * @param dest the destination buffer
     * @param indices the integral parts of the source positions
     * @param coeffs the fractional parts of the source positions
     */
    template <InterpolatorModel M>
    static void fillInterpolated(
        const AudioSpan<const float>& source, AudioSpan<float>& dest,
        absl::Span<const int> indices, absl::Span<const float> coeffs);

    /**
     * @brief Compute the amplitude envelope, applied as a gain to a mono
     * or stereo buffer
     *
     * @param modulationSpan
     */
    void amplitudeEnvelope(absl::Span<float> modulationSpan) noexcept;

    /**
     * @brief Apply the crossfade envelope to a span.
     *
     * @param modulationSpan
     */
    void applyCrossfades(absl::Span<float> modulationSpan) noexcept;
    void resetCrossfades() noexcept;

    /**
     * @brief Amplitude stage for a mono source
     *
     * @param buffer
     */
    void ampStageMono(AudioSpan<float> buffer) noexcept;
    /**
     * @brief Amplitude stage for a stereo source
     *
     * @param buffer
     */
    void ampStageStereo(AudioSpan<float> buffer) noexcept;
    /**
     * @brief Amplitude stage for a mono source
     *
     * @param buffer
     */
    void panStageMono(AudioSpan<float> buffer) noexcept;
    void panStageStereo(AudioSpan<float> buffer) noexcept;
    /**
     * @brief Amplitude stage for a mono source
     *
     * @param buffer
     */
    void filterStageMono(AudioSpan<float> buffer) noexcept;
    void filterStageStereo(AudioSpan<float> buffer) noexcept;
    /**
     * @brief Compute the pitch envelope. This envelope is meant to multiply
     * the frequency parameter for each sample (which translates to floating
     * point intervals for sample-based voices, or phases for generators)
     *
     * @param pitchSpan
     */
    void pitchEnvelope(absl::Span<float> pitchSpan) noexcept;

    /**
     * @brief Remove the voice from the sister ring
     *
     */
    void removeVoiceFromRing() noexcept;

    /**
     * @brief Initialize frequency and gain coefficients for the oscillators.
     */
    void setupOscillatorUnison();
    void updateChannelPowers(AudioSpan<float> buffer);

    /**
     * @brief Modify the voice state and notify any listeners.
     */
    void switchState(State s);

    /**
     * @brief Save the modulation targets to avoid recomputing them in every callback.
     * Must be called during startVoice() ideally.
     */
    void saveModulationTargets(const Region* region) noexcept;

    const NumericId<Voice> id;
    StateListener* stateListener = nullptr;

    Region* region { nullptr };

    State state { State::idle };
    bool noteIsOff { false };

    TriggerEvent triggerEvent;
    absl::optional<int> triggerDelay;

    float speedRatio { 1.0 };
    float pitchRatio { 1.0 };
    float baseVolumedB { 0.0 };
    float baseGain { 1.0 };
    float baseFrequency { 440.0 };

    float floatPositionOffset { 0.0f };
    int sourcePosition { 0 };
    int initialDelay { 0 };
    int age { 0 };

    FilePromisePtr currentPromise { nullptr };

    int samplesPerBlock { config::defaultSamplesPerBlock };
    float sampleRate { config::defaultSampleRate };

    Resources& resources;

    std::vector<FilterHolderPtr> filters;
    std::vector<EQHolderPtr> equalizers;
    std::vector<std::unique_ptr<LFO>> lfos;

    ADSREnvelope<float> egEnvelope;
    float bendStepFactor { centsFactor(1) };

    WavetableOscillator waveOscillators[config::oscillatorsPerVoice];

    // unison of oscillators
    unsigned waveUnisonSize { 0 };
    float waveDetuneRatio[config::oscillatorsPerVoice] {};
    float waveLeftGain[config::oscillatorsPerVoice] {};
    float waveRightGain[config::oscillatorsPerVoice] {};

    Duration dataDuration;
    Duration amplitudeDuration;
    Duration panningDuration;
    Duration filterDuration;

    Voice* nextSisterVoice { this };
    Voice* previousSisterVoice { this };

    fast_real_distribution<float> uniformNoiseDist { -config::uniformNoiseBounds, config::uniformNoiseBounds };
    fast_gaussian_generator<float> gaussianNoiseDist { 0.0f, config::noiseVariance };

    Smoother gainSmoother;
    Smoother bendSmoother;
    Smoother xfadeSmoother;
    void resetSmoothers() noexcept;

    ModMatrix::TargetId amplitudeTarget;
    ModMatrix::TargetId volumeTarget;
    ModMatrix::TargetId panTarget;
    ModMatrix::TargetId positionTarget;
    ModMatrix::TargetId widthTarget;
    ModMatrix::TargetId pitchTarget;

    PowerFollower powerFollower;

    LEAK_DETECTOR(Voice);
};

inline bool sisterVoices(const Voice* lhs, const Voice* rhs)
{
    if (lhs->getAge() != rhs->getAge())
        return false;

    const TriggerEvent& lhsTrigger = lhs->getTriggerEvent();
    const TriggerEvent& rhsTrigger = rhs->getTriggerEvent();

    if (lhsTrigger.number != rhsTrigger.number)
        return false;

    if (lhsTrigger.value != rhsTrigger.value)
        return false;

    if (lhsTrigger.type != rhsTrigger.type)
        return false;

    return true;
}

inline bool voiceOrdering(const Voice* lhs, const Voice* rhs)
{
    if (lhs->getAge() != rhs->getAge())
        return lhs->getAge() > rhs->getAge();

    const TriggerEvent& lhsTrigger = lhs->getTriggerEvent();
    const TriggerEvent& rhsTrigger = rhs->getTriggerEvent();

    if (lhsTrigger.number != rhsTrigger.number)
        return lhsTrigger.number < rhsTrigger.number;

    if (lhsTrigger.value != rhsTrigger.value)
        return lhsTrigger.value < rhsTrigger.value;

    if (lhsTrigger.type != rhsTrigger.type)
        return lhsTrigger.type > rhsTrigger.type;

    return false;
}

} // namespace sfz
