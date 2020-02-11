// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <array>
#include <memory>

namespace sfz {

/*
  * General

  lfoN_freq: Base frequency - Allow modulations at A-rate
  lfoN_phase: Initial phase
  lfoN_delay: Delay
  lfoN_fade: Time to fade-in
  lfoN_count: Number of repetitions - Defective in ARIA? (does not stop)
  lfoN_steps: Length of the step sequence - 1 to 128
  lfoN_stepX: Value of the Xth step of the sequence - -100% to +100%
  lfoN_stepX_onccY: ??? TODO(jpc) check this. override/modulate step in sequence?

    note: LFO evaluates between -1 to +1

    note: make the step sequencer override the main wave when present.
          subwaves are ARIA, step sequencer is Cakewalk, so do our own thing
          which makes the most sense.

  * Subwaveforms
    X: - #1/omitted: the main wave
       - #2-#8: a subwave

    note: if there are gaps in subwaveforms, these subwaveforms which are gaps
          will be initialized and processed.

    example: lfo1_ratio4=1.0 // instanciate implicitly the subs #2 and #3

  lfoN_wave[X]: Wave
  lfoN_offset[X]: DC offset - Add to LFO output; not affected by scale.
  lfoN_ratio[X]: Sub ratio - Frequency = (Ratio * Base Frequency)
  lfoN_scale[X]: Sub scale - Amplitude of sub
*/

class LFO {
public:
    enum Wave {
        kTriangle,
        kSine,
        kPulse75,
        kSquare,
        kPulse25,
        kPulse12_5,
        kRamp,
        kSaw,
    };

    struct Control {
        float freq = 0; // lfoN_freq
        float phase0 = 0; // lfoN_phase
        float delay = 0; // lfoN_delay
        float fade = 0; // lfoN_fade
        unsigned countRepeats = 0; // lfoN_count
        struct Sub {
            Wave wave = kTriangle; // lfoN_wave[X]
            float offset = 0; // lfoN_offset[X]
            float ratio = 1; // lfoN_ratio[X]
            float scale = 1; // lfoN_scale[X]
        };
        struct StepSequence {
            unsigned numSteps = 1;
            enum { maximumSteps = 128 };
            std::array<float, maximumSteps> steps {}; // lfoN_stepX - normalized to unity
        };
        std::unique_ptr<StepSequence> stepSequence;
        unsigned countSubs = 1;
        enum { maximumSubs = 8 };
        std::array<Sub, maximumSubs> sub;
    };

    /**
       Initialize with the given sample rate.
       Run it after instantiating.
     */
    void init(double sampleRate);

    /**
       Attach some control parameters to this LFO.
       The control structure is owned by the caller.
       If the pointer is null, the LFO uses default controls.
     */
    void attachParameters(const Control* ctl);

    /**
       Start processing a LFO as a region is triggered.
       Prepares the delay, phases, fade-in, etc..
     */
    void start();

    /**
       Process a cycle of the oscillator.

       TODO(jpc) frequency modulations
     */
    void process(float* out, unsigned nframes);

private:
    /**
       Evaluate the wave at a given phase.
       Phase must be in the range 0 to 1 excluded.
     */
    template <Wave W>
    static float eval(float phase);

    /**
       Process the nth subwaveform, adding to the buffer.

       This definition is duplicated per each wave, a strategy to avoid a switch
       on wave type inside the frame loop.
     */
    template <Wave W>
    void processWave(unsigned nth, float* out, unsigned nframes);

    /**
       Process the step sequencer, adding to the buffer.
     */
    void processSteps(float* out, unsigned nframes);

private:
    float sampleRate = 0;

    // control
    const Control* control = nullptr;

    // state
    unsigned delayFramesLeft = 0;
    float fadeInPole = 0;
    float fadeInMemory = 0;
    std::array<float, 8> subPhases {};
};

} // namespace sfz
