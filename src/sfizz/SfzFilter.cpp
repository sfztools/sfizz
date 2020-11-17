// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Config.h"
#include "SfzFilter.h"
#include "SfzFilterImpls.hpp"
#include "StringViewHelpers.h"
#include <cstring>
#include "SIMDHelpers.h"
#include "Debug.h"

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

void Filter::prepare(float cutoff, float q, float pksh)
{
    sfzFilterDsp *dsp = P->getDsp(P->fChannels, P->fType);

    if (!dsp)
        return;

    // compute a dummy 1-frame cycle with smoothing off

    float buffer[Impl::maxChannels] = {0};
    float *inout[Impl::maxChannels];
    bool en = dsp->isSmoothingEnabled();

    for (unsigned i = 0; i < Impl::maxChannels; ++i)
        inout[i] = &buffer[i];

    dsp->instanceClear();
    dsp->configureStandard(cutoff, q, pksh);
    dsp->setSmoothingEnabled(false);
    dsp->compute(1, inout, inout);
    dsp->setSmoothingEnabled(en);
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

    dsp->configureStandard(cutoff, q, pksh);
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

        dsp->configureStandard(cutoff[frame], q[frame], pksh[frame]);
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
    ASSERT(channels <= Impl::maxChannels);
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

absl::optional<FilterType> Filter::typeFromName(absl::string_view name)
{
    absl::optional<FilterType> ftype;

    switch (hash(name)) {
    case hash("lpf_1p"): ftype = kFilterLpf1p; break;
    case hash("hpf_1p"): ftype = kFilterHpf1p; break;
    case hash("lpf_2p"): ftype = kFilterLpf2p; break;
    case hash("hpf_2p"): ftype = kFilterHpf2p; break;
    case hash("bpf_2p"): ftype = kFilterBpf2p; break;
    case hash("brf_2p"): ftype = kFilterBrf2p; break;
    case hash("bpf_1p"): ftype = kFilterBpf1p; break;
    case hash("brf_1p"): ftype = kFilterBrf1p; break;
    case hash("apf_1p"): ftype = kFilterApf1p; break;
    case hash("lpf_2p_sv"): ftype = kFilterLpf2pSv; break;
    case hash("hpf_2p_sv"): ftype = kFilterHpf2pSv; break;
    case hash("bpf_2p_sv"): ftype = kFilterBpf2pSv; break;
    case hash("brf_2p_sv"): ftype = kFilterBrf2pSv; break;
    case hash("lpf_4p"): ftype = kFilterLpf4p; break;
    case hash("hpf_4p"): ftype = kFilterHpf4p; break;
    case hash("lpf_6p"): ftype = kFilterLpf6p; break;
    case hash("hpf_6p"): ftype = kFilterHpf6p; break;
    case hash("pink"): ftype = kFilterPink; break;
    case hash("lsh"): ftype = kFilterLsh; break;
    case hash("hsh"): ftype = kFilterHsh; break;
    case hash("bpk_2p"): //fallthrough
    case hash("pkf_2p"): //fallthrough
    case hash("peq"): ftype = kFilterPeq; break;
    }

    return ftype;
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
    EqType fType = kEqNone;
    unsigned fChannels = 1;
    enum { maxChannels = 2 };

    sfzEqPeak fDspPeak;
    sfzEqLshelf fDspLshelf;
    sfzEqHshelf fDspHshelf;

    sfz2chEqPeak fDsp2chPeak;
    sfz2chEqLshelf fDsp2chLshelf;
    sfz2chEqHshelf fDsp2chHshelf;

    sfzFilterDsp *getDsp(unsigned channels, EqType type);

    static constexpr uint32_t idDsp(unsigned channels, EqType type)
    {
        return static_cast<unsigned>(type)|(channels << 16);
    }
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
        EqType ftype = static_cast<EqType>(1);
        while (sfzFilterDsp *dsp = P->getDsp(channels, ftype)) {
            dsp->init(sampleRate);
            ftype = static_cast<EqType>(static_cast<int>(ftype) + 1);
        }
    }
}

void FilterEq::clear()
{
    sfzFilterDsp *dsp = P->getDsp(P->fChannels, P->fType);

    if (dsp)
        dsp->instanceClear();
}

void FilterEq::prepare(float cutoff, float bw, float pksh)
{
    sfzFilterDsp *dsp = P->getDsp(P->fChannels, P->fType);

    if (!dsp)
        return;

    // compute a dummy 1-frame cycle with smoothing off

    float buffer[Impl::maxChannels] = {0};
    float *inout[Impl::maxChannels];
    bool en = dsp->isSmoothingEnabled();

    for (unsigned i = 0; i < Impl::maxChannels; ++i)
        inout[i] = &buffer[i];

    dsp->instanceClear();
    dsp->configureEq(cutoff, bw, pksh);
    dsp->setSmoothingEnabled(false);
    dsp->compute(1, inout, inout);
    dsp->setSmoothingEnabled(en);
}

void FilterEq::process(const float *const in[], float *const out[], float cutoff, float bw, float pksh, unsigned nframes)
{
    unsigned channels = P->fChannels;
    sfzFilterDsp *dsp = P->getDsp(channels, P->fType);

    if (!dsp) {
        for (unsigned c = 0; c < channels; ++c)
            copy<float>({ in[c], nframes }, { out[c], nframes });
        return;
    }

    dsp->configureEq(cutoff, bw, pksh);
    dsp->compute(nframes, const_cast<float **>(in), const_cast<float **>(out));
}

void FilterEq::processModulated(const float *const in[], float *const out[], const float *cutoff, const float *bw, const float *pksh, unsigned nframes)
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

        dsp->configureEq(cutoff[frame], bw[frame], pksh[frame]);
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
    ASSERT(channels <= Impl::maxChannels);
    if (P->fChannels != channels) {
        P->fChannels = channels;
        clear();
    }
}

EqType FilterEq::type() const
{
    return P->fType;
}

void FilterEq::setType(EqType type)
{
    if (P->fType != type) {
        P->fType = type;
        clear();
    }
}

absl::optional<EqType> FilterEq::typeFromName(absl::string_view name)
{
    absl::optional<EqType> ftype;

    switch (hash(name)) {
    case hash("peak"): ftype = kEqPeak; break;
    case hash("lshelf"): ftype = kEqLowShelf; break;
    case hash("hshelf"): ftype = kEqHighShelf; break;
    }

    return ftype;
}

sfzFilterDsp *FilterEq::Impl::getDsp(unsigned channels, EqType type)
{
    switch (idDsp(channels, type)) {
    default: return nullptr;

    case idDsp(1, kEqPeak): return &fDspPeak;
    case idDsp(1, kEqLowShelf): return &fDspLshelf;
    case idDsp(1, kEqHighShelf): return &fDspHshelf;

    case idDsp(2, kEqPeak): return &fDsp2chPeak;
    case idDsp(2, kEqLowShelf): return &fDsp2chLshelf;
    case idDsp(2, kEqHighShelf): return &fDsp2chHshelf;
    }
}

} // namespace sfz
