// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

- [x] gate_attack         Attack time (s)
- [x] gate_release        Release time (s)
- [x] gate_threshold      Threshold (dB)
- [x] gate_stlink         Stereo link (boolean)
- [ ] gate_onccN          Gate manual control (% - 0%=gate open, 100%=gate closed)

  Sfizz Extra

- [x] gate_hold           Hold time (s)

*/

#include "Gate.h"
#include "Opcode.h"
#include "AudioSpan.h"
#include "MathHelpers.h"
#include "OversamplerHelpers.h"
#include "absl/memory/memory.h"

static constexpr int _oversampling = 2;
#define FAUST_UIMACROS 1
#include "gen/gate.cxx"

namespace sfz {
namespace fx {

    struct Gate::Impl {
        faustGate _gate[2];
        bool _stlink { Default::gateSTLink };
        float _inputGain = 1.0;
        AudioBuffer<float, 2> _tempBuffer2x { 2, _oversampling * config::defaultSamplesPerBlock };
        AudioBuffer<float, 2> _gain2x { 2, _oversampling * config::defaultSamplesPerBlock };
        hiir::Downsampler2x<12> _downsampler2x[EffectChannels];
        hiir::Upsampler2x<12> _upsampler2x[EffectChannels];

        #define DEFINE_SET_GET(type, ident, name, var, def, min, max, step) \
            float get_##ident(size_t i) const noexcept { return _gate[i].var; } \
            void set_##ident(size_t i, float value) noexcept { _gate[i].var = value; }
        FAUST_LIST_ACTIVES(DEFINE_SET_GET);
        #undef DEFINE_SET_GET
    };

    Gate::Gate()
        : _impl(new Impl)
    {
        Impl& impl = *_impl;
        for (faustGate& gate : impl._gate)
            gate.instanceResetUserInterface();
    }

    Gate::~Gate()
    {
    }

    void Gate::setSampleRate(double sampleRate)
    {
        Impl& impl = *_impl;
        for (faustGate& gate : impl._gate) {
            gate.classInit(sampleRate);
            gate.instanceConstants(sampleRate);
        }

        for (unsigned c = 0; c < EffectChannels; ++c) {
            impl._downsampler2x[c].set_coefs(OSCoeffs2x);
            impl._upsampler2x[c].set_coefs(OSCoeffs2x);
        }

        clear();
    }

    void Gate::setSamplesPerBlock(int samplesPerBlock)
    {
        Impl& impl = *_impl;
        impl._tempBuffer2x.resize(_oversampling * samplesPerBlock);
        impl._gain2x.resize(_oversampling * samplesPerBlock);
    }

    void Gate::clear()
    {
        Impl& impl = *_impl;
        for (faustGate& gate : impl._gate)
            gate.instanceClear();
    }

    void Gate::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        Impl& impl = *_impl;
        auto inOut2x = AudioSpan<float>(impl._tempBuffer2x).first(_oversampling * nframes);

        absl::Span<float> left2x = inOut2x.getSpan(0);
        absl::Span<float> right2x = inOut2x.getSpan(1);

        impl._upsampler2x[0].process_block(left2x.data(), inputs[0], nframes);
        impl._upsampler2x[1].process_block(right2x.data(), inputs[1], nframes);

        const float inputGain = impl._inputGain;
        for (unsigned i = 0; i < _oversampling * nframes; ++i) {
            left2x[i] *= inputGain;
            right2x[i] *= inputGain;
        }

        if (!impl._stlink) {
            absl::Span<float> leftGain2x = impl._gain2x.getSpan(0);
            absl::Span<float> rightGain2x = impl._gain2x.getSpan(1);

            {
                faustGate& gate = impl._gate[0];
                float* inputs[] = { left2x.data() };
                float* outputs[] = { leftGain2x.data() };
                gate.compute(_oversampling * nframes, inputs, outputs);
            }

            {
                faustGate& gate = impl._gate[1];
                float* inputs[] = { right2x.data() };
                float* outputs[] = { rightGain2x.data() };
                gate.compute(_oversampling * nframes, inputs, outputs);
            }

            for (unsigned i = 0; i < _oversampling * nframes; ++i) {
                left2x[i] *= leftGain2x[i];
                right2x[i] *= rightGain2x[i];
            }
        }
        else {
            absl::Span<float> gateIn2x = impl._gain2x.getSpan(0);
            for (unsigned i = 0; i < _oversampling * nframes; ++i)
                gateIn2x[i] = std::abs(left2x[i]) + std::abs(right2x[1]);

            absl::Span<float> gain2x = impl._gain2x.getSpan(1);

            {
                faustGate& gate = impl._gate[0];
                float* inputs[] = { gateIn2x.data() };
                float* outputs[] = { gain2x.data() };
                gate.compute(_oversampling * nframes, inputs, outputs);
            }

            for (unsigned i = 0; i < _oversampling * nframes; ++i) {
                left2x[i] *= gain2x[i];
                right2x[i] *= gain2x[i];
            }
        }

        impl._downsampler2x[0].process_block(outputs[0], left2x.data(), nframes);
        impl._downsampler2x[1].process_block(outputs[1], right2x.data(), nframes);
    }

    std::unique_ptr<Effect> Gate::makeInstance(absl::Span<const Opcode> members)
    {
        Gate* gate = new Gate;
        std::unique_ptr<Effect> fx { gate };

        Impl& impl = *gate->_impl;

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("gate_attack"):
                {
                    auto value = opc.read(Default::gateAttack);
                    for (size_t c = 0; c < 2; ++c)
                        impl.set_Attack(c, value);
                }
                break;
            case hash("gate_hold"):
                {
                    auto value = opc.read(Default::gateHold);
                    for (size_t c = 0; c < 2; ++c)
                        impl.set_Hold(c, value);
                }
                break;
            case hash("gate_release"):
                {
                    auto value = opc.read(Default::gateRelease);
                    for (size_t c = 0; c < 2; ++c)
                        impl.set_Release(c, value);
                }
                break;
            case hash("gate_threshold"):
                {
                    auto value = opc.read(Default::gateThreshold);
                    for (size_t c = 0; c < 2; ++c)
                        impl.set_Threshold(c, value);
                }
                break;
            case hash("gate_stlink"):
                impl._stlink = opc.read(Default::gateSTLink);
            }
        }

        return fx;
    }

} // namespace fx
} // namespace sfz
