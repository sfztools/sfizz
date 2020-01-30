// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfzFilter.h"
#include "SfzFilterDefs.h"
#include "SfzFilterImpls.cxx"
#include <cstring>
#include <cassert>

namespace sfz {

template <unsigned NCh>
struct Filter<NCh>::Impl {
    FilterType fType = kFilterNone;

    sfzLpf1p<NCh> fDspLpf1p;
    sfzLpf2p<NCh> fDspLpf2p;
    sfzLpf4p<NCh> fDspLpf4p;
    sfzLpf6p<NCh> fDspLpf6p;
    sfzHpf1p<NCh> fDspHpf1p;
    sfzHpf2p<NCh> fDspHpf2p;
    sfzHpf4p<NCh> fDspHpf4p;
    sfzHpf6p<NCh> fDspHpf6p;
    sfzBpf1p<NCh> fDspBpf1p;
    sfzBpf2p<NCh> fDspBpf2p;
    sfzBpf4p<NCh> fDspBpf4p;
    sfzBpf6p<NCh> fDspBpf6p;
    sfzApf1p<NCh> fDspApf1p;
    sfzBrf1p<NCh> fDspBrf1p;
    sfzBrf2p<NCh> fDspBrf2p;
    sfzPink<NCh> fDspPink;
    sfzLpf2pSv<NCh> fDspLpf2pSv;
    sfzHpf2pSv<NCh> fDspHpf2pSv;
    sfzBpf2pSv<NCh> fDspBpf2pSv;
    sfzBrf2pSv<NCh> fDspBrf2pSv;
    sfzLsh<NCh> fDspLsh;
    sfzHsh<NCh> fDspHsh;
    sfzPeq<NCh> fDspPeq;

    template <class F> static void process(F &filter, const float *const in[NCh], float *const out[NCh], float cutoff, float q, float pksh, unsigned nframes);
    template <class F> static void processModulated(F &filter, const float *const in[NCh], float *const out[NCh], const float *cutoff, const float *q, const float *pksh, unsigned nframes);
};

template <unsigned NCh>
Filter<NCh>::Filter()
    : P{new Impl}
{
}

template <unsigned NCh>
Filter<NCh>::~Filter()
{
}

template <unsigned NCh>
void Filter<NCh>::init(double sampleRate)
{
    P->fDspLpf1p.init(sampleRate);
    P->fDspLpf2p.init(sampleRate);
    P->fDspLpf4p.init(sampleRate);
    P->fDspLpf6p.init(sampleRate);
    P->fDspHpf1p.init(sampleRate);
    P->fDspHpf2p.init(sampleRate);
    P->fDspHpf4p.init(sampleRate);
    P->fDspHpf6p.init(sampleRate);
    P->fDspBpf1p.init(sampleRate);
    P->fDspBpf2p.init(sampleRate);
    P->fDspBpf4p.init(sampleRate);
    P->fDspBpf6p.init(sampleRate);
    P->fDspApf1p.init(sampleRate);
    P->fDspBrf1p.init(sampleRate);
    P->fDspBrf2p.init(sampleRate);
    P->fDspPink.init(sampleRate);
    P->fDspLpf2pSv.init(sampleRate);
    P->fDspHpf2pSv.init(sampleRate);
    P->fDspBpf2pSv.init(sampleRate);
    P->fDspBrf2pSv.init(sampleRate);
    P->fDspLsh.init(sampleRate);
    P->fDspHsh.init(sampleRate);
    P->fDspPeq.init(sampleRate);
}

template <unsigned NCh>
void Filter<NCh>::clear()
{
    switch (P->fType) {
    case kFilterNone: break;
    case kFilterApf1p: P->fDspApf1p.instanceClear(); break;
    case kFilterBpf1p: P->fDspBpf1p.instanceClear(); break;
    case kFilterBpf2p: P->fDspBpf2p.instanceClear(); break;
    case kFilterBpf4p: P->fDspBpf4p.instanceClear(); break;
    case kFilterBpf6p: P->fDspBpf6p.instanceClear(); break;
    case kFilterBrf1p: P->fDspBrf1p.instanceClear(); break;
    case kFilterBrf2p: P->fDspBrf2p.instanceClear(); break;
    case kFilterHpf1p: P->fDspHpf1p.instanceClear(); break;
    case kFilterHpf2p: P->fDspHpf2p.instanceClear(); break;
    case kFilterHpf4p: P->fDspHpf4p.instanceClear(); break;
    case kFilterHpf6p: P->fDspHpf6p.instanceClear(); break;
    case kFilterLpf1p: P->fDspLpf1p.instanceClear(); break;
    case kFilterLpf2p: P->fDspLpf2p.instanceClear(); break;
    case kFilterLpf4p: P->fDspLpf4p.instanceClear(); break;
    case kFilterLpf6p: P->fDspLpf6p.instanceClear(); break;
    case kFilterPink: P->fDspPink.instanceClear(); break;
    case kFilterLpf2pSv: P->fDspLpf2pSv.instanceClear(); break;
    case kFilterHpf2pSv: P->fDspHpf2pSv.instanceClear(); break;
    case kFilterBpf2pSv: P->fDspBpf2pSv.instanceClear(); break;
    case kFilterBrf2pSv: P->fDspBrf2pSv.instanceClear(); break;
    case kFilterLsh: P->fDspLsh.instanceClear(); break;
    case kFilterHsh: P->fDspHsh.instanceClear(); break;
    case kFilterPeq: P->fDspPeq.instanceClear(); break;
    }
}

template <unsigned NCh>
template <class F>
void Filter<NCh>::Impl::process(F &filter, const float *const in[NCh], float *const out[NCh], float cutoff, float q, float pksh, unsigned nframes)
{
    filter.setCutoff(cutoff);
    filter.setQ(q);
    filter.setPkShGain(pksh);
    filter.compute(nframes, const_cast<float **>(in), const_cast<float **>(out));
}

template <unsigned NCh>
void Filter<NCh>::process(const float *const in[NCh], float *const out[NCh], float cutoff, float q, float pksh, unsigned nframes)
{
    if (P->fType == kFilterNone) {
        for (unsigned c = 0; c < NCh; ++c) {
            const float *ch_in = in[c];
            float *ch_out = out[c];
            if (ch_in != ch_out)
                std::memcpy(ch_out, ch_in, nframes * sizeof(float));
        }
        return;
    }

    switch (P->fType) {
    case kFilterApf1p: P->process(P->fDspApf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf1p: P->process(P->fDspBpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf2p: P->process(P->fDspBpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf4p: P->process(P->fDspBpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf6p: P->process(P->fDspBpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf1p: P->process(P->fDspBrf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf2p: P->process(P->fDspBrf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf1p: P->process(P->fDspHpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf2p: P->process(P->fDspHpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf4p: P->process(P->fDspHpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf6p: P->process(P->fDspHpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf1p: P->process(P->fDspLpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf2p: P->process(P->fDspLpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf4p: P->process(P->fDspLpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf6p: P->process(P->fDspLpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterPink: P->process(P->fDspPink, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf2pSv: P->process(P->fDspLpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf2pSv: P->process(P->fDspHpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf2pSv: P->process(P->fDspBpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf2pSv: P->process(P->fDspBrf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLsh: P->process(P->fDspLsh, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHsh: P->process(P->fDspHsh, in, out, cutoff, q, pksh, nframes); break;
    case kFilterPeq: P->process(P->fDspPeq, in, out, cutoff, q, pksh, nframes); break;
    default: assert(false);
    }
}

template <unsigned NCh>
template <class F>
void Filter<NCh>::Impl::processModulated(F &filter, const float *const in[NCh], float *const out[NCh], const float *cutoff, const float *q, const float *pksh, unsigned nframes)
{
    unsigned frame = 0;

    while (frame < nframes) {
        unsigned current = nframes - frame;

        if (current > kFilterControlInterval)
            current = kFilterControlInterval;

        const float *current_in[NCh];
        float *current_out[NCh];

        for (unsigned c = 0; c < NCh; ++c) {
            current_in[c] = in[c] + frame;
            current_out[c] = out[c] + frame;
        }

        filter.setCutoff(cutoff[frame]);
        filter.setQ(q[frame]);
        filter.setPkShGain(pksh[frame]);
        filter.compute(current, const_cast<float **>(current_in), const_cast<float **>(current_out));

        frame += current;
    }
}

template <unsigned NCh>
void Filter<NCh>::processModulated(const float *const in[NCh], float *const out[NCh], const float *cutoff, const float *q, const float *pksh, unsigned nframes)
{
    if (P->fType == kFilterNone) {
        for (unsigned c = 0; c < NCh; ++c) {
            const float *ch_in = in[c];
            float *ch_out = out[c];
            if (ch_in != ch_out)
                std::memcpy(ch_out, ch_in, nframes * sizeof(float));
        }
        return;
    }

    switch (P->fType) {
    case kFilterApf1p: P->processModulated(P->fDspApf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf1p: P->processModulated(P->fDspBpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf2p: P->processModulated(P->fDspBpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf4p: P->processModulated(P->fDspBpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf6p: P->processModulated(P->fDspBpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf1p: P->processModulated(P->fDspBrf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf2p: P->processModulated(P->fDspBrf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf1p: P->processModulated(P->fDspHpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf2p: P->processModulated(P->fDspHpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf4p: P->processModulated(P->fDspHpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf6p: P->processModulated(P->fDspHpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf1p: P->processModulated(P->fDspLpf1p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf2p: P->processModulated(P->fDspLpf2p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf4p: P->processModulated(P->fDspLpf4p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf6p: P->processModulated(P->fDspLpf6p, in, out, cutoff, q, pksh, nframes); break;
    case kFilterPink: P->processModulated(P->fDspPink, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLpf2pSv: P->processModulated(P->fDspLpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHpf2pSv: P->processModulated(P->fDspHpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBpf2pSv: P->processModulated(P->fDspBpf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterBrf2pSv: P->processModulated(P->fDspBrf2pSv, in, out, cutoff, q, pksh, nframes); break;
    case kFilterLsh: P->processModulated(P->fDspLsh, in, out, cutoff, q, pksh, nframes); break;
    case kFilterHsh: P->processModulated(P->fDspHsh, in, out, cutoff, q, pksh, nframes); break;
    case kFilterPeq: P->processModulated(P->fDspPeq, in, out, cutoff, q, pksh, nframes); break;
    default: assert(false);
    }
}

template <unsigned NCh>
FilterType Filter<NCh>::type() const
{
    return P->fType;
}

template <unsigned NCh>
void Filter<NCh>::setType(FilterType type)
{
    if (P->fType != type) {
        P->fType = type;
        clear();
    }
}

template class Filter<1>;
template class Filter<2>;

} // namespace sfz
