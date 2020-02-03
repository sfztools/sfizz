// -*- mode: faust; -*-

declare author "Jean Pierre Cimalando";
declare license "BSD-2-Clause";

import("stdfaust.lib");
rbj = library("rbj_filters.dsp");

//-------------------------------------------------------------------------
// Biquad filter from normalized coefficients
//     b0,b1,b2,a1,a2 : normalized coefficients
//-------------------------------------------------------------------------
biquad(b0,b1,b2,a1,a2) = fi.iir((b0,b1,b2),(a1,a2));

//-------------------------------------------------------------------------
// Biquad filter using smoothed coefficients
//     s : a smoothing function applied to each coefficient
//     b0,b1,b2,a1,a2 : normalized coefficients
//-------------------------------------------------------------------------
smoothBiquad(s,b0,b1,b2,a1,a2) = biquad(b0:s,b1:s,b2:s,a1:s,a2:s);

//-------------------------------------------------------------------------
// RBJ filter of a specific type using smoothed coefficients
//     s : a smoothing function applied to each coefficient
//     f : cutoff frequency
//     g : gain in decibel, for peaking and shelving types only
//     q : height of the resonant peak in linear units
//-------------------------------------------------------------------------
rbjLpfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).LPF,x) : smoothBiquad(s);
rbjHpfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).HPF,x) : smoothBiquad(s);
rbjBpfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).BPF,x) : smoothBiquad(s);
rbjNotchSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).notch,x) : smoothBiquad(s);
rbjApfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).APF,x) : smoothBiquad(s);
rbjPeakingEqSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).peakingEQ,x) : smoothBiquad(s);
rbjPeakingNotchSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).peakNotch,x) : smoothBiquad(s);
rbjLowShelfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).lowShelf,x) : smoothBiquad(s);
rbjHighShelfSmooth(s,f,g,q,x) = (rbj.filtercoeff(f,g,q).highShelf,x) : smoothBiquad(s);

//-------------------------------------------------------------------------
// 1-pole low-pass filter
//     s : a smoothing function applied to each coefficient
//     f : cutoff frequency
//-------------------------------------------------------------------------
lp1Smooth(s,f) = fi.iir((1-p),(0-p)) with {
  p = exp(-2.*ma.PI*f/ma.SR) : s;
};

//-------------------------------------------------------------------------
// 1-pole high-pass filter
//     s : a smoothing function applied to each coefficient
//     f : cutoff frequency
//-------------------------------------------------------------------------
hp1Smooth(s,f) = fi.iir((0.5*(1.+p),-0.5*(1+p)),(0.-p)) with {
  p = exp(-2.*ma.PI*f/ma.SR) : s;
};

//-------------------------------------------------------------------------
// 1-pole all-pass filter
//     s : a smoothing function applied to each coefficient
//     f : cutoff frequency
//-------------------------------------------------------------------------
ap1Smooth(s,f) = fi.iir((a,1.),(a)) with {
  a = (-1.+2.*ma.PI*f/ma.SR) : s;
};

//------------------------------------------------------------------------------
// Example
//------------------------------------------------------------------------------
// process = rbjLpfSmooth(si.smoo, cutoff, 0.0, resonance : ba.db2linear) with {
//   cutoff = hslider("[1] Cutoff [unit:Hz] [scale:log]", 440.0, 50.0, 10000.0, 1.0);
//   resonance = hslider("[2] Resonance [unit:dB]", 0.0, 0.0, 40.0, 0.1);
// };
