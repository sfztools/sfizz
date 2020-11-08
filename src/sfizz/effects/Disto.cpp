// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

- [x] disto_tone
- [ ] disto_tone_oncc
- [x] disto_depth
- [ ] disto_depth_oncc
- [x] disto_stages
- [x] disto_dry
- [ ] disto_dry_oncc
- [x] disto_wet
- [ ] disto_wet_oncc
*/

#include "Disto.h"
#include "Opcode.h"
#include "Config.h"
#include "MathHelpers.h"
#include <hiir/Upsampler2xFpu.h>
#include <hiir/Downsampler2xFpu.h>
#include <absl/types/span.h>
#include <cmath>

static constexpr int _oversampling = 8;
#define FAUST_UIMACROS 1
#include "gen/disto_stage.cxx"

namespace sfz {
namespace fx {

struct Disto::Impl {
    enum { maxStages = 4 };

    float _samplePeriod { 1.0f / config::defaultSampleRate };
    float _tone { Default::distoTone.value };
    float _depth { Default::distoDepth.value };
    float _dry { Default::effect.value };
    float _wet { Default::effect.value };
    unsigned _numStages = { Default::distoStages.value };

    float _toneLpfMem[EffectChannels] = {};
    faustDisto _stages[EffectChannels][Default::maxDistoStages];

    hiir::Upsampler2xFpu<12> _up2x[EffectChannels];
    hiir::Upsampler2xFpu<4> _up4x[EffectChannels];
    hiir::Upsampler2xFpu<3> _up8x[EffectChannels];

    hiir::Downsampler2xFpu<12> _down2x[EffectChannels];
    hiir::Downsampler2xFpu<4> _down4x[EffectChannels];
    hiir::Downsampler2xFpu<3> _down8x[EffectChannels];

    std::unique_ptr<float[]> _temp8x[2];

    // use the same formula as reverb
    float toneCutoff() const noexcept
    {
        float mk = 21.0f + _tone * 1.08f;
        return 440.0f * std::exp2((mk - 69.0f) * (1.0f / 12.0f));
    }

    #define DEFINE_SET_GET(type, ident, name, var, def, min, max, step) \
        float get_##ident(size_t c, size_t s) const noexcept { return _stages[c][s].var; } \
        void set_##ident(size_t c, size_t s, float value) noexcept { _stages[c][s].var = value; }
    FAUST_LIST_ACTIVES(DEFINE_SET_GET);
    #undef DEFINE_SET_GET
};

Disto::Disto()
    : _impl(new Impl)
{
    Impl& impl = *_impl;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        for (faustDisto& stage : impl._stages[c])
            stage.init(config::defaultSampleRate);
    }
}

Disto::~Disto()
{
}

void Disto::setSampleRate(double sampleRate)
{
    Impl& impl = *_impl;
    impl._samplePeriod = 1.0 / sampleRate;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        for (faustDisto& stage : impl._stages[c]) {
            stage.classInit(sampleRate);
            stage.instanceConstants(sampleRate);
        }
    }

    static constexpr double coefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    static constexpr double coefs4x[4] = { 0.042448989488488006, 0.17072114107630679, 0.39329183835224008, 0.74569514831986694 };
    static constexpr double coefs8x[3] = { 0.055748680811302048, 0.24305119574153092, 0.6466991311926823 };

    for (unsigned c = 0; c < EffectChannels; ++c) {
        impl._down2x[c].set_coefs(coefs2x);
        impl._down4x[c].set_coefs(coefs4x);
        impl._down8x[c].set_coefs(coefs8x);
        impl._up2x[c].set_coefs(coefs2x);
        impl._up4x[c].set_coefs(coefs4x);
        impl._up8x[c].set_coefs(coefs8x);
    }
}

void Disto::setSamplesPerBlock(int samplesPerBlock)
{
    Impl& impl = *_impl;

    for (std::unique_ptr<float[]>& temp : impl._temp8x)
        temp.reset(new float[8 * samplesPerBlock]);
}

void Disto::clear()
{
    Impl& impl = *_impl;
    for (unsigned c = 0; c < EffectChannels; ++c) {
        for (faustDisto& stage : impl._stages[c])
            stage.instanceClear();
    }

    for (unsigned c = 0; c < EffectChannels; ++c) {
        impl._toneLpfMem[c] = 0.0f;
        impl._up2x[c].clear_buffers();
        impl._up4x[c].clear_buffers();
        impl._up8x[c].clear_buffers();
        impl._down2x[c].clear_buffers();
        impl._down4x[c].clear_buffers();
        impl._down8x[c].clear_buffers();
    }
}

void Disto::process(const float* const inputs[], float* const outputs[], unsigned nframes)
{
    // Note(jpc): assumes `inputs` and `outputs` to be different buffers

    Impl& impl = *_impl;
    const float dry = impl._dry;
    const float wet = impl._wet;
    const float depth = impl._depth;
    const float toneLpfPole = std::exp(float(-2.0 * M_PI) * impl.toneCutoff() * impl._samplePeriod);

    for (unsigned c = 0; c < EffectChannels; ++c) {
        // compute LPF
        absl::Span<const float> channelIn(inputs[c], nframes);
        absl::Span<float> lpfOut(outputs[c], nframes);
        float lpfMem = impl._toneLpfMem[c];
        for (unsigned i = 0; i < nframes; ++i) {
            // Note(jpc) apply `dry` gain, note there is no output if
            //           `dry=0 wet=<any>`, it is the same behavior as reference
            lpfMem = channelIn[i] * dry * (1.0f - toneLpfPole) + lpfMem * toneLpfPole;
            lpfOut[i] = lpfMem;
        }
        impl._toneLpfMem[c] = lpfMem;

        // upsample to 8x
        absl::Span<float> temp[2] = {
            absl::Span<float>(impl._temp8x[0].get(), 8 * nframes),
            absl::Span<float>(impl._temp8x[1].get(), 8 * nframes),
        };
        impl._up2x[c].process_block(temp[0].data(), lpfOut.data(), nframes);
        impl._up4x[c].process_block(temp[1].data(), temp[0].data(), 2 * nframes);
        impl._up8x[c].process_block(temp[0].data(), temp[1].data(), 4 * nframes);
        absl::Span<float> upsamplerOut = temp[0];

        // run disto stages
        absl::Span<float> stageInOut = upsamplerOut;
        for (unsigned s = 0, numStages = impl._numStages; s < numStages; ++s) {
            // set depth parameter (TODO modulation)
            impl.set_Depth(c, s, depth);
            //
            float *faustIn[] = { stageInOut.data() };
            float *faustOut[] = { stageInOut.data() };
            impl._stages[c][s].compute(8 * nframes, faustIn, faustOut);
        }

        // downsample to 1x
        impl._down8x[c].process_block(temp[1].data(), stageInOut.data(), 4 * nframes);
        impl._down4x[c].process_block(temp[0].data(), temp[1].data(), 2 * nframes);
        impl._down2x[c].process_block(outputs[c], temp[0].data(), nframes);

        // dry/wet mix
        absl::Span<float> mixOut(outputs[c], nframes);
        for (unsigned i = 0; i < nframes; ++i)
            mixOut[i] = mixOut[i] * wet + channelIn[i] * (1.0f - wet);
    }
}

std::unique_ptr<Effect> Disto::makeInstance(absl::Span<const Opcode> members)
{
    Disto* disto = new Disto;
    std::unique_ptr<Effect> fx { disto };

    Impl& impl = *disto->_impl;

    for (const Opcode& opc : members) {
        switch (opc.lettersOnlyHash) {
        case hash("disto_tone"):
            if (auto value = opc.read(Default::distoTone))
                impl._tone = *value;
            break;
        case hash("disto_depth"):
            if (auto value = opc.read(Default::distoDepth))
                impl._depth = *value;
            break;
        case hash("disto_stages"):
            if (auto value = opc.read(Default::distoStages))
                impl._numStages = *value;
            break;
        case hash("disto_dry"):
            if (auto value = opc.read(Default::effect))
                impl._dry = *value * 0.01f;
            break;
        case hash("disto_wet"):
            if (auto value = opc.read(Default::effect))
                impl._wet = *value * 0.01f;
            break;
        }
    }

    return fx;
}

} // namespace sfz
} // namespace fx
