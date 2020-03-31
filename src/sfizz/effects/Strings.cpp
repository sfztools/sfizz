// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz


/**
   Note(jpc): implementation status

- [x] strings_number
- [ ] strings_wet_oncc

Extensions
- [x] strings_wet
 */

#include "Strings.h"
#include "StringsPrivate.h"
#include "Opcode.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "absl/memory/memory.h"
#include <cmath>

namespace sfz {
namespace fx {

    struct Strings::ResonantString {
        Bw2BPF bpf;
        WgResonator res;
    };

    Strings::Strings()
        : _strings(new ResonantString[MaximumNumStrings])
    {
    }

    Strings::~Strings()
    {
    }

    void Strings::setSampleRate(double sampleRate)
    {
        for (unsigned i = 0, n = _numStrings; i < n; ++i) {
            ResonantString& rs = _strings[i];
            rs.bpf.init(sampleRate);
            rs.res.init(sampleRate);

            int midiNote = i + 24;
            double midiFrequency = 440.0 * std::exp2((midiNote - 69) * (1.0 / 12.0));

            // 1 Hz works decently as compromise of selectivity/speed
            double bpfBandwidth = 1.0;
            rs.bpf.setCutoff(
                midiFrequency - 0.5 * bpfBandwidth,
                midiFrequency + 0.5 * bpfBandwidth);

            rs.res.setFrequency(midiFrequency);

            // TODO(jpc) find how to adjust the string feedbacks
            //     for now set a fixed release time for all strings
            double releaseTime = 50e-3;
            double releaseFeedback = std::exp(-6.91 / (releaseTime * sampleRate));
            rs.res.setFeedback(releaseFeedback);
        }
    }

    void Strings::setSamplesPerBlock(int samplesPerBlock)
    {
        _tempBuffer.resize(samplesPerBlock);
    }

    void Strings::clear()
    {
        for (unsigned i = 0, n = _numStrings; i < n; ++i) {
            ResonantString& rs = _strings[i];
            rs.bpf.clear();
            rs.res.clear();
        }
    }

    void Strings::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        auto inputL = absl::MakeConstSpan(inputs[0], nframes);
        auto inputR = absl::MakeConstSpan(inputs[1], nframes);

        // mix down the stereo signal to create the resonator excitation source
        absl::Span<float> resInput = _tempBuffer.getSpan(0).first(nframes);
        sfz::applyGain<float>(M_SQRT1_2, inputL, resInput);
        sfz::multiplyAdd<float>(M_SQRT1_2, inputR, resInput);

        // generate the strings summed into a common buffer
        absl::Span<float> resOutput = _tempBuffer.getSpan(1).first(nframes);
        sfz::fill(resOutput, 0.0f);

        for (unsigned is = 0, ns = _numStrings; is < ns; ++is) {
            ResonantString& rs = _strings[is];
            for (unsigned i = 0; i < nframes; ++i) {
                float sample = resInput[i];
                sample = rs.bpf.process(sample);
                sample = rs.res.process(sample);
                resOutput[i] += sample;
            }
        }

        // TODO(jpc) damping of the high frequencies
        //     it's easiest apply individual gains to resonating strings
        //     or pass resonator output through LPF

        // mix the resonator into the output
        auto outputL = absl::MakeSpan(outputs[0], nframes);
        auto outputR = absl::MakeSpan(outputs[1], nframes);

        constexpr float resAttenuate = 1e-3; // need significant attenuation, here -60dB

        absl::Span<float> wet = _tempBuffer.getSpan(2).first(nframes);
        sfz::fill(wet, 0.01f * resAttenuate *_wet); // TOD strings_wet_oncc modulation...

        sfz::copy(inputL, outputL);
        sfz::copy(inputR, outputR);
        sfz::multiplyAdd<float>(wet, resOutput, outputL);
        sfz::multiplyAdd<float>(wet, resOutput, outputR);
    }

    std::unique_ptr<Effect> Strings::makeInstance(absl::Span<const Opcode> members)
    {
        auto fx = absl::make_unique<Strings>();

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("strings_number"):
                setValueFromOpcode(opc, fx->_numStrings, {0, MaximumNumStrings});
                break;
            case hash("strings_wet"):
                setValueFromOpcode(opc, fx->_wet, {0.0f, 100.0f});
                break;
            }
        }

        return CXX11_MOVE(fx);
    }

} // namespace fx
} // namespace sfz
