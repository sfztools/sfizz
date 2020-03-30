// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "MathHelpers.h"
#include <cmath>

namespace sfz {
namespace fx {

    // Butterworth 2nd order bandpass (faust -double -os)
    /*
      import("stdfaust.lib");
      process = fi.bandpass(1, loF, hiF) with {
          loF = hslider("[1] Lo frequency [unit:Hz]", 1, 0, 1000, 1);
          hiF = hslider("[2] Hi frequency [unit:Hz]", 1, 0, 1000, 1);
      };
     */
    class Bw2BPF {
    private:
        typedef float FAUSTFLOAT;

    public:
        /**
         * @brief Initialize.
         */
        void init(double sampleRate)
        {
            fConst0 = sampleRate;
            fConst1 = (2.0 / fConst0);
            fConst2 = (2.0 * fConst0);
            fConst3 = (3.1415926535897931 / fConst0);
            fConst4 = (0.5 / fConst0);
            fConst5 = (4.0 * power2(fConst0));
            fConst6 = power2((1.0 / fConst0));
            fConst7 = (2.0 * fConst6);
            clear();
        }

        /**
         * @brief Clear the memory of the filter.
         */
        void clear()
        {
            for (int l0 = 0; (l0 < 3); l0 = (l0 + 1)) {
                fRec0[l0] = 0.0;
            }
        }

        /**
         * @brief Set the BPF low and high frequencies for -3dB response.
         *
         * The center frequency is (loF+hiF)/2.
         */
        void setCutoff(double loF, double hiF)
        {
            fControl[0] = std::tan((fConst3 * double(hiF)));
            fControl[1] = power2(std::sqrt((fConst5 * (fControl[0] * std::tan((fConst3 * double(loF)))))));
            fControl[2] = ((fConst2 * fControl[0]) - (fConst4 * (fControl[1] / fControl[0])));
            fControl[3] = (fConst6 * fControl[1]);
            fControl[4] = (fConst1 * fControl[2]);
            fControl[5] = ((fControl[3] + fControl[4]) + 4.0);
            fControl[6] = (fConst1 * (fControl[2] / fControl[5]));
            fControl[7] = (1.0 / fControl[5]);
            fControl[8] = ((fConst7 * fControl[1]) + -8.0);
            fControl[9] = (fControl[3] + (4.0 - fControl[4]));
            fControl[10] = (0.0 - fControl[6]);
        }

        /**
         * @brief Process the next filtered sample.
         */
        FAUSTFLOAT process(FAUSTFLOAT input)
        {
            fRec0[0] = (double(input) - (fControl[7] * ((fControl[8] * fRec0[1]) + (fControl[9] * fRec0[2]))));
            FAUSTFLOAT output = FAUSTFLOAT(((fControl[6] * fRec0[0]) + (fControl[10] * fRec0[2])));
            fRec0[2] = fRec0[1];
            fRec0[1] = fRec0[0];
            return output;
        }

    private:
        double fRec0[3] {};
        double fControl[11] {};
        double fConst0 {};
        double fConst1 {};
        double fConst2 {};
        double fConst3 {};
        double fConst4 {};
        double fConst5 {};
        double fConst6 {};
        double fConst7 {};
    };

    //--------------------------------------------------------------------------

    // Waveguide resonator (faust -os)
    /*
        import("stdfaust.lib");
        process = fi.nlf2(f, r) : (_,!) with {
          f = hslider("[1] Resonance frequency [unit:Hz]", 1, 0, 1000, 1);
          r = hslider("[2] Resonance feedback", 0, 0, 1, 1e-3);
        };
     */
    class WgResonator {
    private:
        typedef float FAUSTFLOAT;

    public:
        /**
         * @brief Initialize.
         */
        void init(float sampleRate)
        {
            fConst0 = (6.28318548f / sampleRate);
            clear();
        }

        /**
         * @brief Clear the memory of the resonator.
         */
        void clear()
        {
            for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
                fRec0[l0] = 0.0f;
            }
            for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
                fRec1[l1] = 0.0f;
            }
        }

        /**
         * @brief Set the resonance frequency.
         */
        void setFrequency(float frequency)
        {
            fControl[1] = (fConst0 * float(frequency));
            fControl[2] = std::sin(fControl[1]);
            fControl[3] = std::cos(fControl[1]);
        }

        /**
         * @brief Set the resonance feedback.
         */
        void setFeedback(float feedback)
        {
            fControl[0] = float(feedback);
        }

        /**
         * @brief Process the next resonance sample.
         */
        FAUSTFLOAT process(FAUSTFLOAT input)
        {
            fRec0[0] = (fControl[0] * ((fControl[2] * fRec1[1]) + (fControl[3] * fRec0[1])));
            fRec1[0] = ((float(input) + (fControl[3] * fRec1[1])) - (fControl[2] * fRec0[1]));
            FAUSTFLOAT output = FAUSTFLOAT(fRec0[0]);
            fRec0[1] = fRec0[0];
            fRec1[1] = fRec1[0];
            return output;
        }

    private:
        float fRec0[2] {};
        float fRec1[2] {};
        float fControl[4];
        float fConst0 {};
    };

} // namespace sfz
} // namespace fx
