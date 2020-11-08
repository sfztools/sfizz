// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
   Note(jpc): this effect is book-only, mentioned but not documented

   Note(jpc): implementation status

- [x] rectify_mode
- [x] rectify
- [ ] rectify_oncc
 */

#include "Rectify.h"
#include "Opcode.h"

namespace sfz {
namespace fx {

    Rectify::Rectify()
    {
    }

    void Rectify::setSampleRate(double sampleRate)
    {
        (void)sampleRate;

        static constexpr double coefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };

        for (unsigned c = 0; c < EffectChannels; ++c) {
            _downsampler2x[c].set_coefs(coefs2x);
            _upsampler2x[c].set_coefs(coefs2x);
        }
    }

    void Rectify::setSamplesPerBlock(int samplesPerBlock)
    {
        _tempBuffer.resize(samplesPerBlock);
    }

    void Rectify::clear()
    {
        for (unsigned c = 0; c < EffectChannels; ++c) {
            _downsampler2x[c].clear_buffers();
            _upsampler2x[c].clear_buffers();
        }
    }

    void Rectify::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        // Note(jpc) I define opcode `rectify` to be a mix amount.
        //           half-wave rectification is achieved simply by halving it.
        const float baseAmount = _amount * (_full ? 1.0f : 0.5f);

        absl::Span<float> amounts = _tempBuffer.getSpan(0);
        std::fill(amounts.begin(), amounts.end(), baseAmount);

        for (unsigned c = 0; c < EffectChannels; ++c) {
            const float *input = inputs[c];
            float *output = outputs[c];

            auto &up2x = _upsampler2x[c];
            auto &down2x = _downsampler2x[c];

            for (unsigned i = 0; i < nframes; ++i) {
                const float amount = normalizePercents(amounts[i]);
                float in = input[i];

                float in2x[2];
                up2x.process_sample(in2x[0], in2x[1], in);

                float out2x[2];
                out2x[0] = amount * std::fabs(in2x[0]) + (1.0f - amount) * in2x[0];
                out2x[1] = amount * std::fabs(in2x[1]) + (1.0f - amount) * in2x[1];

                output[i] = down2x.process_sample(out2x);
            }
        }
    }

    std::unique_ptr<Effect> Rectify::makeInstance(absl::Span<const Opcode> members)
    {
        Rectify* rectify = new Rectify;
        std::unique_ptr<Effect> fx { rectify };

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("rectify_mode"):
                if (opc.value == "full")
                    rectify->_full = true;
                else if (opc.value == "half")
                    rectify->_full = false;
                break;
            case hash("rectify"):
                if (auto value = opc.read(Default::rectify))
                    rectify->_amount = *value;
                break;
            }
        }

        return fx;
    }

} // namespace fx
} // namespace sfz
