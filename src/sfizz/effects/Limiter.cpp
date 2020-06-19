// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

   Complete (no opcodes)
*/

#include "Limiter.h"
#include "Opcode.h"
#include "AudioSpan.h"
#include "absl/memory/memory.h"

static constexpr int _oversampling = 2;
#include "gen/limiter.cpp"

namespace sfz {
namespace fx {

    Limiter::Limiter()
        : _limiter(new faustLimiter)
    {
        _limiter->instanceResetUserInterface();
    }

    Limiter::~Limiter()
    {
    }

    void Limiter::setSampleRate(double sampleRate)
    {
        _limiter->classInit(sampleRate);
        _limiter->instanceConstants(sampleRate);

        static constexpr double coefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };

        for (unsigned c = 0; c < EffectChannels; ++c) {
            _downsampler2x[c].set_coefs(coefs2x);
            _upsampler2x[c].set_coefs(coefs2x);
        }

        clear();
    }

    void Limiter::setSamplesPerBlock(int samplesPerBlock)
    {
        _tempBuffer2x.resize(2 * samplesPerBlock);
    }

    void Limiter::clear()
    {
        _limiter->instanceClear();
    }

    void Limiter::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        auto inOut2x = AudioSpan<float>( _tempBuffer2x).first(2 * nframes);

        for (unsigned c = 0; c < EffectChannels; ++c)
            _upsampler2x[c].process_block(inOut2x.getSpan(c).data(), inputs[c], nframes);

        _limiter->compute(2 * nframes, inOut2x, inOut2x);

        for (unsigned c = 0; c < EffectChannels; ++c)
            _downsampler2x[c].process_block(outputs[c], inOut2x.getSpan(c).data(), nframes);
    }

    std::unique_ptr<Effect> Limiter::makeInstance(absl::Span<const Opcode> members)
    {
        auto fx = absl::make_unique<Limiter>();

        for (const Opcode& opc : members) {
            // no opcodes
            (void)opc;
        }

        return std::unique_ptr<Effect> { fx.release() };
    }

} // namespace fx
} // namespace sfz
