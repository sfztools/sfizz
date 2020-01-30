// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

struct dsp {};
struct Meta {};
struct UI {};

#include "gen/filters/sfzApf1p.cxx"
#include "gen/filters/sfzBpf1p.cxx"
#include "gen/filters/sfzBpf2p.cxx"
#include "gen/filters/sfzBpf4p.cxx"
#include "gen/filters/sfzBpf6p.cxx"
#include "gen/filters/sfzBrf1p.cxx"
#include "gen/filters/sfzBrf2p.cxx"
#include "gen/filters/sfzHpf1p.cxx"
#include "gen/filters/sfzHpf2p.cxx"
#include "gen/filters/sfzHpf4p.cxx"
#include "gen/filters/sfzHpf6p.cxx"
#include "gen/filters/sfzLpf1p.cxx"
#include "gen/filters/sfzLpf2p.cxx"
#include "gen/filters/sfzLpf4p.cxx"
#include "gen/filters/sfzLpf6p.cxx"
#include "gen/filters/sfzPink.cxx"
#include "gen/filters/sfzLpf2pSv.cxx"
#include "gen/filters/sfzHpf2pSv.cxx"
#include "gen/filters/sfzBpf2pSv.cxx"
#include "gen/filters/sfzBrf2pSv.cxx"
#include "gen/filters/sfzLsh.cxx"
#include "gen/filters/sfzHsh.cxx"
#include "gen/filters/sfzPeq.cxx"

#include "gen/filters/sfz2chApf1p.cxx"
#include "gen/filters/sfz2chBpf1p.cxx"
#include "gen/filters/sfz2chBpf2p.cxx"
#include "gen/filters/sfz2chBpf4p.cxx"
#include "gen/filters/sfz2chBpf6p.cxx"
#include "gen/filters/sfz2chBrf1p.cxx"
#include "gen/filters/sfz2chBrf2p.cxx"
#include "gen/filters/sfz2chHpf1p.cxx"
#include "gen/filters/sfz2chHpf2p.cxx"
#include "gen/filters/sfz2chHpf4p.cxx"
#include "gen/filters/sfz2chHpf6p.cxx"
#include "gen/filters/sfz2chLpf1p.cxx"
#include "gen/filters/sfz2chLpf2p.cxx"
#include "gen/filters/sfz2chLpf4p.cxx"
#include "gen/filters/sfz2chLpf6p.cxx"
#include "gen/filters/sfz2chPink.cxx"
#include "gen/filters/sfz2chLpf2pSv.cxx"
#include "gen/filters/sfz2chHpf2pSv.cxx"
#include "gen/filters/sfz2chBpf2pSv.cxx"
#include "gen/filters/sfz2chBrf2pSv.cxx"
#include "gen/filters/sfz2chLsh.cxx"
#include "gen/filters/sfz2chHsh.cxx"
#include "gen/filters/sfz2chPeq.cxx"

template <class F> struct sfzFilter : public F {
    void setCutoff(float v) { F::fCutoff = v; }
    void setQ(float v) { F::fQ = v; }
    void setPkShGain(float) {}
};

template <class F> struct sfzFilterNoQ : public F {
    void setCutoff(float v) { F::fCutoff = v; }
    void setQ(float) {}
    void setPkShGain(float) {}
};

template <class F> struct sfzFilterNoCutoff : public F {
    void setCutoff(float) {}
    void setQ(float) {}
    void setPkShGain(float) {}
};

template <class F> struct sfzFilterEq : public F {
    void setCutoff(float v) { F::fCutoff = v; }
    void setQ(float v) { F::fQ = v; }
    void setPkShGain(float v) { F::fPkShGain = v; }
};

template <unsigned NCh> struct sfzLpf1p;
template <unsigned NCh> struct sfzLpf2p;
template <unsigned NCh> struct sfzLpf4p;
template <unsigned NCh> struct sfzLpf6p;
template <unsigned NCh> struct sfzHpf1p;
template <unsigned NCh> struct sfzHpf2p;
template <unsigned NCh> struct sfzHpf4p;
template <unsigned NCh> struct sfzHpf6p;
template <unsigned NCh> struct sfzBpf1p;
template <unsigned NCh> struct sfzBpf2p;
template <unsigned NCh> struct sfzBpf4p;
template <unsigned NCh> struct sfzBpf6p;
template <unsigned NCh> struct sfzApf1p;
template <unsigned NCh> struct sfzBrf1p;
template <unsigned NCh> struct sfzBrf2p;
template <unsigned NCh> struct sfzPink;
template <unsigned NCh> struct sfzLpf2pSv;
template <unsigned NCh> struct sfzHpf2pSv;
template <unsigned NCh> struct sfzBpf2pSv;
template <unsigned NCh> struct sfzBrf2pSv;
template <unsigned NCh> struct sfzLsh;
template <unsigned NCh> struct sfzHsh;
template <unsigned NCh> struct sfzPeq;

template<> struct sfzLpf1p<1> : public sfzFilterNoQ<faustLpf1p> {};
template<> struct sfzLpf2p<1> : public sfzFilter<faustLpf2p> {};
template<> struct sfzLpf4p<1> : public sfzFilter<faustLpf4p> {};
template<> struct sfzLpf6p<1> : public sfzFilter<faustLpf6p> {};
template<> struct sfzHpf1p<1> : public sfzFilterNoQ<faustHpf1p> {};
template<> struct sfzHpf2p<1> : public sfzFilter<faustHpf2p> {};
template<> struct sfzHpf4p<1> : public sfzFilter<faustHpf4p> {};
template<> struct sfzHpf6p<1> : public sfzFilter<faustHpf6p> {};
template<> struct sfzBpf1p<1> : public sfzFilterNoQ<faustBpf1p> {};
template<> struct sfzBpf2p<1> : public sfzFilter<faustBpf2p> {};
template<> struct sfzBpf4p<1> : public sfzFilter<faustBpf4p> {};
template<> struct sfzBpf6p<1> : public sfzFilter<faustBpf6p> {};
template<> struct sfzApf1p<1> : public sfzFilterNoQ<faustApf1p> {};
template<> struct sfzBrf1p<1> : public sfzFilterNoQ<faustBrf1p> {};
template<> struct sfzBrf2p<1> : public sfzFilter<faustBrf2p> {};
template<> struct sfzPink<1> : public sfzFilterNoCutoff<faustPink> {};
template<> struct sfzLpf2pSv<1> : public sfzFilter<faustLpf2pSv> {};
template<> struct sfzHpf2pSv<1> : public sfzFilter<faustHpf2pSv> {};
template<> struct sfzBpf2pSv<1> : public sfzFilter<faustBpf2pSv> {};
template<> struct sfzBrf2pSv<1> : public sfzFilter<faustBrf2pSv> {};
template<> struct sfzLsh<1> : public sfzFilterEq<faustLsh> {};
template<> struct sfzHsh<1> : public sfzFilterEq<faustHsh> {};
template<> struct sfzPeq<1> : public sfzFilterEq<faustPeq> {};

template<> struct sfzLpf1p<2> : public sfzFilterNoQ<faust2chLpf1p> {};
template<> struct sfzLpf2p<2> : public sfzFilter<faust2chLpf2p> {};
template<> struct sfzLpf4p<2> : public sfzFilter<faust2chLpf4p> {};
template<> struct sfzLpf6p<2> : public sfzFilter<faust2chLpf6p> {};
template<> struct sfzHpf1p<2> : public sfzFilterNoQ<faust2chHpf1p> {};
template<> struct sfzHpf2p<2> : public sfzFilter<faust2chHpf2p> {};
template<> struct sfzHpf4p<2> : public sfzFilter<faust2chHpf4p> {};
template<> struct sfzHpf6p<2> : public sfzFilter<faust2chHpf6p> {};
template<> struct sfzBpf1p<2> : public sfzFilterNoQ<faust2chBpf1p> {};
template<> struct sfzBpf2p<2> : public sfzFilter<faust2chBpf2p> {};
template<> struct sfzBpf4p<2> : public sfzFilter<faust2chBpf4p> {};
template<> struct sfzBpf6p<2> : public sfzFilter<faust2chBpf6p> {};
template<> struct sfzApf1p<2> : public sfzFilterNoQ<faust2chApf1p> {};
template<> struct sfzBrf1p<2> : public sfzFilterNoQ<faust2chBrf1p> {};
template<> struct sfzBrf2p<2> : public sfzFilter<faust2chBrf2p> {};
template<> struct sfzPink<2> : public sfzFilterNoCutoff<faust2chPink> {};
template<> struct sfzLpf2pSv<2> : public sfzFilter<faust2chLpf2pSv> {};
template<> struct sfzHpf2pSv<2> : public sfzFilter<faust2chHpf2pSv> {};
template<> struct sfzBpf2pSv<2> : public sfzFilter<faust2chBpf2pSv> {};
template<> struct sfzBrf2pSv<2> : public sfzFilter<faust2chBrf2pSv> {};
template<> struct sfzLsh<2> : public sfzFilterEq<faust2chLsh> {};
template<> struct sfzHsh<2> : public sfzFilterEq<faust2chHsh> {};
template<> struct sfzPeq<2> : public sfzFilterEq<faust2chPeq> {};
