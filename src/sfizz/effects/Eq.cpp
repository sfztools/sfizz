// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz


/**
   Note(jpc): implementation status

- [x] eq_type
- [x] eq_freq
- [ ] eq_freq_oncc
- [x] eq_bw
- [ ] eq_bw_oncc
- [x] eq_gain
- [ ] eq_gain_oncc

 */

#include "Eq.h"
#include "Opcode.h"
#include "SIMDHelpers.h"
#include "Debug.h"
#include "absl/memory/memory.h"

namespace sfz {
namespace fx {
    Eq::Eq(const EQDescription& desc)
        : _desc(desc)
    {
        _filter.setType(desc.type);
        _filter.setChannels(2);
    }

    void Eq::setSampleRate(double sampleRate)
    {
        _filter.init(sampleRate);
        prepareFilter();
    }

    void Eq::setSamplesPerBlock(int samplesPerBlock)
    {
        _tempBuffer.resize(samplesPerBlock);
    }

    void Eq::clear()
    {
        _filter.clear();
        prepareFilter();
    }

    void Eq::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        absl::Span<float> cutoff = _tempBuffer.getSpan(0).first(nframes);
        absl::Span<float> bw = _tempBuffer.getSpan(1).first(nframes);
        absl::Span<float> pksh = _tempBuffer.getSpan(2).first(nframes);

        sfz::fill(cutoff, _desc.frequency);
        sfz::fill(bw, _desc.bandwidth);
        sfz::fill(pksh, _desc.gain);

        _filter.processModulated(inputs, outputs, cutoff.data(), bw.data(), pksh.data(), nframes);
    }

    std::unique_ptr<Effect> Eq::makeInstance(absl::Span<const Opcode> members)
    {
        EQDescription desc;

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("eq_freq"):
                if (auto value = opc.read(Default::eqFrequency))
                    desc.frequency = *value;
                break;
            case hash("eq_bw"):
                if (auto value = opc.read(Default::eqBandwidth))
                    desc.bandwidth = *value;
                break;
            case hash("eq_gain"):
                if (auto value = opc.read(Default::eqGain))
                    desc.gain = *value;
                break;
            case hash("eq_type"):
                {
                    absl::optional<EqType> ftype = sfz::FilterEq::typeFromName(opc.value);
                    if (ftype)
                        desc.type = *ftype;
                    else {
                        desc.type = EqType::kEqNone;
                        DBG("Unknown EQ type: " << std::string(opc.value));
                    }
                    break;
                }
            }
        }

        Eq* eq = new Eq(desc);
        std::unique_ptr<Effect> fx { eq };
        return fx;
    }

    void Eq::prepareFilter()
    {
        _filter.prepare(_desc.frequency, _desc.bandwidth, _desc.gain);
    }

} // namespace fx
} // namespace sfz
