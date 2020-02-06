// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <memory>

namespace sfz {

enum FilterType : int;

/**
   Multi-mode filter for SFZ v2
   Available for mono and stereo. (NCh=1, NCh=2)

   Parameters:
     `cutoff`: it's the opcode `filN_cutoff` (Hz)
     `q`: it's the opcode `filN_resonance` (dB)
     `pksh`: it's the opcode `filN_gain` (dB)
 */
template <unsigned NCh>
class Filter {
public:
    Filter();
    ~Filter();

    /**
       Set up the filter constants.
       Run it exactly once after instantiating.
     */
    void init(double sampleRate);

    /**
       Reinitialize the filter memory to zeros.
     */
    void clear();

    /**
       Process one cycle of the filter without modulating cutoff or Q.
       `cutoff` is a frequency expressed in Hz.
       `q` is a resonance expressed in dB.
       `pksh` is a peak/shelf gain expressed in dB.
       `in[i]` and `out[i]` may refer to identical buffers, for in-place processing
     */
    void process(const float *const in[NCh], float *const out[NCh], float cutoff, float q, float pksh, unsigned nframes);

    /**
       Process one cycle of the filter with cutoff and Q values varying over time.
       `cutoff` is a frequency expressed in Hz.
       `q` is a resonance expressed in dB.
       `pksh` is a peak/shelf gain expressed in dB.
       `in[i]` and `out[i]` may refer to identical buffers, for in-place processing
     */
    void processModulated(const float *const in[NCh], float *const out[NCh], const float *cutoff, const float *q, const float *pksh, unsigned nframes);

    /**
       Get the type of filter.
     */
    FilterType type() const;

    /**
       Set the type of filter.
     */
    void setType(FilterType type);

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};

enum FilterType : int {
    kFilterNone,
    kFilterApf1p,
    kFilterBpf1p,
    kFilterBpf2p,
    kFilterBpf4p,
    kFilterBpf6p,
    kFilterBrf1p,
    kFilterBrf2p,
    kFilterHpf1p,
    kFilterHpf2p,
    kFilterHpf4p,
    kFilterHpf6p,
    kFilterLpf1p,
    kFilterLpf2p,
    kFilterLpf4p,
    kFilterLpf6p,
    kFilterPink,
    kFilterLpf2pSv,
    kFilterHpf2pSv,
    kFilterBpf2pSv,
    kFilterBrf2pSv,
    kFilterLsh,
    kFilterHsh,
    kFilterPeq,
};

/**
   Equalizer filter for SFZ v1
   Available for mono and stereo. (NCh=1, NCh=2)

   Parameters:
     `cutoff`: it's the opcode `egN_freq` (Hz)
     `bw`: it's the opcode `eqN_bw` (octave)
     `pksh`: it's the opcode `eqN_gain` (dB)
 */
template <unsigned NCh>
class FilterEq {
public:
    FilterEq();
    ~FilterEq();

    /**
       Set up the filter constants.
       Run it exactly once after instantiating.
     */
    void init(double sampleRate);

    /**
       Reinitialize the filter memory to zeros.
     */
    void clear();

    /**
       Process one cycle of the filter without modulating cutoff or bandwidth.
       `cutoff` is a frequency expressed in Hz.
       `bw` is a bandwidth expressed in octaves.
       `pksh` is a peak/shelf gain expressed in dB.
       `in[i]` and `out[i]` may refer to identical buffers, for in-place processing
     */
    void process(const float *const in[NCh], float *const out[NCh], float cutoff, float bw, float pksh, unsigned nframes);

    /**
       Process one cycle of the filter with cutoff and bandwidth values varying over time.
       `cutoff` is a frequency expressed in Hz.
       `bw` is a bandwidth expressed in octaves.
       `pksh` is a peak/shelf gain expressed in dB.
       `in[i]` and `out[i]` may refer to identical buffers, for in-place processing
     */
    void processModulated(const float *const in[NCh], float *const out[NCh], const float *cutoff, const float *bw, const float *pksh, unsigned nframes);

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};

} // namespace sfz
