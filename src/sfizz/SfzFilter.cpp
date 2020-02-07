// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "SfzFilter.h"
#include "SfzFilterImpls.cxx"
#include <cstring>
#include <cassert>
#include "SIMDHelpers.h"

namespace sfz {

//------------------------------------------------------------------------------
// SFZ v2 multi-mode filter

struct Filter::Impl {
    FilterType fType = kFilterNone;
    unsigned fChannels = 1;
    enum { maxChannels = 2 };

    sfzLpf1p fDspLpf1p;
    sfzLpf2p fDspLpf2p;
    sfzLpf4p fDspLpf4p;
    sfzLpf6p fDspLpf6p;
    sfzHpf1p fDspHpf1p;
    sfzHpf2p fDspHpf2p;
    sfzHpf4p fDspHpf4p;
    sfzHpf6p fDspHpf6p;
    sfzBpf1p fDspBpf1p;
    sfzBpf2p fDspBpf2p;
    sfzBpf4p fDspBpf4p;
    sfzBpf6p fDspBpf6p;
    sfzApf1p fDspApf1p;
    sfzBrf1p fDspBrf1p;
    sfzBrf2p fDspBrf2p;
    sfzPink fDspPink;
    sfzLpf2pSv fDspLpf2pSv;
    sfzHpf2pSv fDspHpf2pSv;
    sfzBpf2pSv fDspBpf2pSv;
    sfzBrf2pSv fDspBrf2pSv;
    sfzLsh fDspLsh;
    sfzHsh fDspHsh;
    sfzPeq fDspPeq;

    sfz2chLpf1p fDsp2chLpf1p;
    sfz2chLpf2p fDsp2chLpf2p;
    sfz2chLpf4p fDsp2chLpf4p;
    sfz2chLpf6p fDsp2chLpf6p;
    sfz2chHpf1p fDsp2chHpf1p;
    sfz2chHpf2p fDsp2chHpf2p;
    sfz2chHpf4p fDsp2chHpf4p;
    sfz2chHpf6p fDsp2chHpf6p;
    sfz2chBpf1p fDsp2chBpf1p;
    sfz2chBpf2p fDsp2chBpf2p;
    sfz2chBpf4p fDsp2chBpf4p;
    sfz2chBpf6p fDsp2chBpf6p;
    sfz2chApf1p fDsp2chApf1p;
    sfz2chBrf1p fDsp2chBrf1p;
    sfz2chBrf2p fDsp2chBrf2p;
    sfz2chPink fDsp2chPink;
    sfz2chLpf2pSv fDsp2chLpf2pSv;
    sfz2chHpf2pSv fDsp2chHpf2pSv;
    sfz2chBpf2pSv fDsp2chBpf2pSv;
    sfz2chBrf2pSv fDsp2chBrf2pSv;
    sfz2chLsh fDsp2chLsh;
    sfz2chHsh fDsp2chHsh;
    sfz2chPeq fDsp2chPeq;

    sfzFilterDsp *getDsp(unsigned channels, FilterType type);

    static constexpr uint32_t idDsp(unsigned channels, FilterType type)
    {
        return static_cast<unsigned>(type)|(channels << 16);
    }
};

Filter::Filter()
    : P{new Impl}
{
}

Filter::~Filter()
{
}

void Filter::init(double sampleRate)
{
    for (unsigned channels = 1; channels <= Impl::maxChannels; ++channels) {
        FilterType ftype = static_cast<FilterType>(1);
        while (sfzFilterDsp *dsp = P->getDsp(channels, ftype)) {
            dsp->init(sampleRate);
            ftype = static_cast<FilterType>(static_cast<int>(ftype) + 1);
        }
    }
}

void Filter::clear()
{
    sfzFilterDsp *dsp = P->getDsp(P->fChannels, P->fType);

    if (dsp)
        dsp->instanceClear();
}

void Filter::process(const float *const in[], float *const out[], float cutoff, float q, float pksh, unsigned nframes)
{
    unsigned channels = P->fChannels;
    sfzFilterDsp *dsp = P->getDsp(channels, P->fType);

    if (!dsp) {
        for (unsigned c = 0; c < channels; ++c)
            copy<float>({ in[c], nframes }, { out[c], nframes });
        return;
    }

    dsp->setCutoff(cutoff);
    dsp->setQ(q);
    dsp->setPkShGain(pksh);
    dsp->compute(nframes, const_cast<float **>(in), const_cast<float **>(out));
}

void Filter::processModulated(const float *const in[], float *const out[], const float *cutoff, const float *q, const float *pksh, unsigned nframes)
{
    unsigned channels = P->fChannels;
    sfzFilterDsp *dsp = P->getDsp(channels, P->fType);

    if (!dsp) {
        for (unsigned c = 0; c < channels; ++c)
            copy<float>({ in[c], nframes }, { out[c], nframes });
        return;
    }

    unsigned frame = 0;
    while (frame < nframes) {
        unsigned current = nframes - frame;

        if (current > config::filterControlInterval)
            current = config::filterControlInterval;

        const float *current_in[Impl::maxChannels];
        float *current_out[Impl::maxChannels];

        for (unsigned c = 0; c < channels; ++c) {
            current_in[c] = in[c] + frame;
            current_out[c] = out[c] + frame;
        }

        dsp->setCutoff(cutoff[frame]);
        dsp->setQ(q[frame]);
        dsp->setPkShGain(pksh[frame]);
        dsp->compute(current, const_cast<float **>(current_in), const_cast<float **>(current_out));

        frame += current;
    }
}

unsigned Filter::channels() const
{
    return P->fChannels;
}

void Filter::setChannels(unsigned channels)
{
    assert(channels < Impl::maxChannels);
    if (P->fChannels != channels) {
        P->fChannels = channels;
        clear();
    }
}

FilterType Filter::type() const
{
    return P->fType;
}

void Filter::setType(FilterType type)
{
    if (P->fType != type) {
        P->fType = type;
        clear();
    }
}

sfzFilterDsp *Filter::Impl::getDsp(unsigned channels, FilterType type)
{
    switch (idDsp(channels, type)) {
    default: return nullptr;

    case idDsp(1, kFilterApf1p): return &fDspApf1p;
    case idDsp(1, kFilterBpf1p): return &fDspBpf1p;
    case idDsp(1, kFilterBpf2p): return &fDspBpf2p;
    case idDsp(1, kFilterBpf4p): return &fDspBpf4p;
    case idDsp(1, kFilterBpf6p): return &fDspBpf6p;
    case idDsp(1, kFilterBrf1p): return &fDspBrf1p;
    case idDsp(1, kFilterBrf2p): return &fDspBrf2p;
    case idDsp(1, kFilterHpf1p): return &fDspHpf1p;
    case idDsp(1, kFilterHpf2p): return &fDspHpf2p;
    case idDsp(1, kFilterHpf4p): return &fDspHpf4p;
    case idDsp(1, kFilterHpf6p): return &fDspHpf6p;
    case idDsp(1, kFilterLpf1p): return &fDspLpf1p;
    case idDsp(1, kFilterLpf2p): return &fDspLpf2p;
    case idDsp(1, kFilterLpf4p): return &fDspLpf4p;
    case idDsp(1, kFilterLpf6p): return &fDspLpf6p;
    case idDsp(1, kFilterPink): return &fDspPink;
    case idDsp(1, kFilterLpf2pSv): return &fDspLpf2pSv;
    case idDsp(1, kFilterHpf2pSv): return &fDspHpf2pSv;
    case idDsp(1, kFilterBpf2pSv): return &fDspBpf2pSv;
    case idDsp(1, kFilterBrf2pSv): return &fDspBrf2pSv;
    case idDsp(1, kFilterLsh): return &fDspLsh;
    case idDsp(1, kFilterHsh): return &fDspHsh;
    case idDsp(1, kFilterPeq): return &fDspPeq;

    case idDsp(2, kFilterApf1p): return &fDsp2chApf1p;
    case idDsp(2, kFilterBpf1p): return &fDsp2chBpf1p;
    case idDsp(2, kFilterBpf2p): return &fDsp2chBpf2p;
    case idDsp(2, kFilterBpf4p): return &fDsp2chBpf4p;
    case idDsp(2, kFilterBpf6p): return &fDsp2chBpf6p;
    case idDsp(2, kFilterBrf1p): return &fDsp2chBrf1p;
    case idDsp(2, kFilterBrf2p): return &fDsp2chBrf2p;
    case idDsp(2, kFilterHpf1p): return &fDsp2chHpf1p;
    case idDsp(2, kFilterHpf2p): return &fDsp2chHpf2p;
    case idDsp(2, kFilterHpf4p): return &fDsp2chHpf4p;
    case idDsp(2, kFilterHpf6p): return &fDsp2chHpf6p;
    case idDsp(2, kFilterLpf1p): return &fDsp2chLpf1p;
    case idDsp(2, kFilterLpf2p): return &fDsp2chLpf2p;
    case idDsp(2, kFilterLpf4p): return &fDsp2chLpf4p;
    case idDsp(2, kFilterLpf6p): return &fDsp2chLpf6p;
    case idDsp(2, kFilterPink): return &fDsp2chPink;
    case idDsp(2, kFilterLpf2pSv): return &fDsp2chLpf2pSv;
    case idDsp(2, kFilterHpf2pSv): return &fDsp2chHpf2pSv;
    case idDsp(2, kFilterBpf2pSv): return &fDsp2chBpf2pSv;
    case idDsp(2, kFilterBrf2pSv): return &fDsp2chBrf2pSv;
    case idDsp(2, kFilterLsh): return &fDsp2chLsh;
    case idDsp(2, kFilterHsh): return &fDsp2chHsh;
    case idDsp(2, kFilterPeq): return &fDsp2chPeq;
    }
}

//------------------------------------------------------------------------------
// SFZ v1 equalizer filter


struct FilterEq::Impl {
    unsigned fChannels = 1;
    enum { maxChannels = 2 };

    sfzEq fDsp;
    sfz2chEq fDsp2ch;

    sfzFilterDsp *getDsp(unsigned channels);
};

FilterEq::FilterEq()
    : P{new Impl}
{
}

FilterEq::~FilterEq()
{
}

void FilterEq::init(double sampleRate)
{
    for (unsigned channels = 1; channels <= Impl::maxChannels; ++channels) {
        sfzFilterDsp *dsp = P->getDsp(channels);
        dsp->init(sampleRate);
    }
}

void FilterEq::clear()
{
    sfzFilterDsp *dsp = P->getDsp(P->fChannels);

    if (dsp)
        dsp->instanceClear();
}

void FilterEq::process(const float *const in[], float *const out[], float cutoff, float bw, float pksh, unsigned nframes)
{
    unsigned channels = P->fChannels;
    sfzFilterDsp *dsp = P->getDsp(channels);

    if (!dsp) {
        for (unsigned c = 0; c < channels; ++c)
            copy<float>({ in[c], nframes }, { out[c], nframes });
        return;
    }

    dsp->setCutoff(cutoff);
    dsp->setBandwidth(bw);
    dsp->setPkShGain(pksh);
    dsp->compute(nframes, const_cast<float **>(in), const_cast<float **>(out));
}

void FilterEq::processModulated(const float *const in[], float *const out[], const float *cutoff, const float *bw, const float *pksh, unsigned nframes)
{
    unsigned channels = P->fChannels;
    sfzFilterDsp *dsp = P->getDsp(channels);

    if (!dsp) {
        for (unsigned c = 0; c < channels; ++c)
            copy<float>({ in[c], nframes }, { out[c], nframes });
        return;
    }

    unsigned frame = 0;
    while (frame < nframes) {
        unsigned current = nframes - frame;

        if (current > config::filterControlInterval)
            current = config::filterControlInterval;

        const float *current_in[Impl::maxChannels];
        float *current_out[Impl::maxChannels];

        for (unsigned c = 0; c < channels; ++c) {
            current_in[c] = in[c] + frame;
            current_out[c] = out[c] + frame;
        }

        dsp->setCutoff(cutoff[frame]);
        dsp->setBandwidth(bw[frame]);
        dsp->setPkShGain(pksh[frame]);
        dsp->compute(current, const_cast<float **>(current_in), const_cast<float **>(current_out));

        frame += current;
    }
}


unsigned FilterEq::channels() const
{
    return P->fChannels;
}

void FilterEq::setChannels(unsigned channels)
{
    assert(channels < Impl::maxChannels);
    if (P->fChannels != channels) {
        P->fChannels = channels;
        clear();
    }
}

sfzFilterDsp *FilterEq::Impl::getDsp(unsigned channels)
{
    switch (channels) {
    default: return nullptr;

    case 1: return &fDsp;
    case 2: return &fDsp2ch;
    }
}

} // namespace sfz
