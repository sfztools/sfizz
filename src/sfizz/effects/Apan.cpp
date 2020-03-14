// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

- [x] apan_waveform
- [x] apan_freq
- [ ] apan_freq_oncc
- [x] apan_phase
- [ ] apan_phase_oncc
- [x] apan_dry
- [ ] apan_dry_oncc
- [x] apan_wet
- [ ] apan_wet_oncc
- [x] apan_depth
- [ ] apan_depth_oncc
*/

#include "Apan.h"
#include "Macros.h"
#include "CommonLFO.h"
#include "Opcode.h"
#include <limits>
#include <cmath>

namespace sfz {
namespace fx {

    void Apan::setSampleRate(double sampleRate)
    {
        _samplePeriod = 1.0 / sampleRate;
    }

    void Apan::setSamplesPerBlock(int samplesPerBlock)
    {
        _lfoOutLeft.reset(new float[samplesPerBlock]);
        _lfoOutRight.reset(new float[samplesPerBlock]);
    }

    void Apan::clear()
    {
        _lfoPhase = 0.0;
    }

    void Apan::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        float dry = _dry;
        float wet = _wet;
        float depth = _depth;
        float* modL = _lfoOutLeft.get();
        float* modR = _lfoOutRight.get();

        computeLfos(modL, modR, nframes);

        const float* inL = inputs[0];
        const float* inR = inputs[1];
        float* outL = outputs[0];
        float* outR = outputs[1];

        for (unsigned i = 0; i < nframes; ++i) {
            float modDD = depth * 0.5f * (modL[i] - modR[i]); // LFO in ±depth

            // uses a linear pan law
            float gainL = 1 - modDD;
            float gainR = 1 + modDD;

            outL[i] = inL[i] * (gainL * wet + dry);
            outR[i] = inR[i] * (gainR * wet + dry);
        }
    }

    std::unique_ptr<Effect> Apan::makeInstance(absl::Span<const Opcode> members)
    {
        std::unique_ptr<Apan> fx { new Apan };

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("apan_waveform"):
                if (auto value = readOpcode(opc.value, Default::apanWaveformRange))
                    fx->_lfoWave = *value;
                break;
            case hash("apan_freq"):
                if (auto value = readOpcode(opc.value, Default::apanFrequencyRange))
                    fx->_lfoFrequency = *value;
                break;
            case hash("apan_phase"):
                if (auto value = readOpcode(opc.value, Default::apanPhaseRange)) {
                    float phase = *value / 360.0f;
                    phase -= (int)phase;
                    fx->_lfoPhaseOffset = phase;
                }
                break;
            case hash("apan_dry"):
                if (auto value = readOpcode(opc.value, Default::apanLevelRange))
                    fx->_dry = *value / 100.0f;
                break;
            case hash("apan_wet"):
                if (auto value = readOpcode(opc.value, Default::apanLevelRange))
                    fx->_wet = *value / 100.0f;
                break;
            case hash("apan_depth"):
                if (auto value = readOpcode(opc.value, Default::apanLevelRange))
                    fx->_depth = *value / 100.0f;
                break;
            }
        }

        return CXX11_MOVE(fx);
    }

    void Apan::computeLfos(float* left, float* right, unsigned nframes)
    {
        switch (_lfoWave) {
        #define CASE(X) case lfo::X:                          \
            computeLfos<lfo::X>(left, right, nframes); break;
        default:
        CASE(kTriangle)
        CASE(kSine)
        CASE(kPulse75)
        CASE(kSquare)
        CASE(kPulse25)
        CASE(kPulse12_5)
        CASE(kRamp)
        CASE(kSaw)
        #undef CASE
        }
    }

    template <int Wave> void Apan::computeLfos(float* left, float* right, unsigned nframes)
    {
        float samplePeriod = _samplePeriod;
        float frequency = _lfoFrequency;
        float offset = _lfoPhaseOffset;
        float phaseLeft = _lfoPhase;

        for (unsigned i = 0; i < nframes; ++i) {
            float phaseRight = phaseLeft + offset;
            phaseRight -= (int)phaseRight;

            left[i] = lfo::evaluateAtPhase<Wave>(phaseLeft);
            right[i] = lfo::evaluateAtPhase<Wave>(phaseRight);

            phaseLeft += frequency * samplePeriod;
            phaseLeft -= (int)phaseLeft;
        }

        _lfoPhase = phaseLeft;
    }

} // namespace fx
} // namespace sfz
