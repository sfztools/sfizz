// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "LeakDetector.h"
#include "Region.h"
#include "MidiState.h"
#include <absl/types/span.h>
namespace sfz {
/**
 * @brief Describe an attack/delay/sustain/release envelope that can
 * produce its coefficient in a blockwise manner for SIMD-type operations.
 *
 * @tparam Type the underlying type
 */
template <class Type>
class ADSREnvelope {
public:
    ADSREnvelope() = default;
    /**
     * @brief Resets the ADSR envelope given a Region, the current midi state, and a delay and
     * trigger velocity
     *
     * @param desc
     * @param region
     * @param state
     * @param delay
     * @param velocity
     */
    void reset(const EGDescription& desc, const Region& region, const MidiState& state, int delay, float velocity, float sampleRate) noexcept;
    /**
     * @brief Get the next value for the envelope
     *
     * @return Type
     */
    Type getNextValue() noexcept;
    /**
     * @brief Get a block of values for the envelope. This method tries hard to be efficient
     * and hopefully it is.
     *
     * @param output
     */
    void getBlock(absl::Span<Type> output) noexcept;
    /**
     * @brief Set the release time for the envelope
     *
     * @param timeInSeconds
     */
    void setReleaseTime(Type timeInSeconds) noexcept;
    /**
     * @brief Start the envelope release after a delay.
     *
     * @param releaseDelay the delay before releasing in samples
     */
    void startRelease(int releaseDelay) noexcept;
    /**
     * @brief Is the envelope smoothing?
     *
     * @return true
     * @return false
     */
    bool isSmoothing() const noexcept;
    /**
     * @brief Is the envelope released?
     *
     * @return true
     * @return false
     */
    bool isReleased() const noexcept;
    /**
     * @brief Get the remaining delay samples
     *
     * @return int
     */
    int getRemainingDelay() const noexcept;

private:
    float sampleRate { config::defaultSampleRate };
    Type secondsToSamples (Type timeInSeconds) const noexcept;
    Type secondsToLinRate (Type timeInSeconds) const noexcept;
    Type secondsToExpRate (Type timeInSeconds) const noexcept;

    enum class State {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Done
    };
    State currentState { State::Done };
    Type currentValue { 0.0 };
    int delay { 0 };
    Type attackStep { 0 };
    Type decayRate { 0 };
    Type releaseRate { 0 };
    int hold { 0 };
    Type start { 0 };
    Type peak { 0 };
    Type sustain { 0 };
    Type sustainThreshold { config::virtuallyZero };
    int releaseDelay { 0 };
    bool shouldRelease { false };
    bool freeRunning { false };
    LEAK_DETECTOR(ADSREnvelope);
};

}
