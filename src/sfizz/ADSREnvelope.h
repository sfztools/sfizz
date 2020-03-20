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
     * @brief Start the envelope release after a delay.
     *
     * @param releaseDelay the delay before releasing in samples
     * @param fastRelease whether the release should be fast (i.e. 0 or so) or
     *                    follow the release duration that was set when
     *                    initializing the envelope
     */
    void startRelease(int releaseDelay, bool fastRelease = false) noexcept;
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
    Type step { 0.0 };
    int delay { 0 };
    int attack { 0 };
    int decay { 0 };
    int release { 0 };
    int hold { 0 };
    Type start { 0 };
    Type peak { 0 };
    Type sustain { 0 };
    int releaseDelay { 0 };
    bool shouldRelease { false };
    bool freeRunning { false };
    LEAK_DETECTOR(ADSREnvelope);
};

}
