/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faustEqPeak_H__
#define  __faustEqPeak_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS
#define FAUSTCLASS faustEqPeak
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustEqPeak : public sfzFilterDsp {

 public:

	int fSampleRate;
	double fConst0;
	double fConst1;
	double fConst2;
	FAUSTFLOAT fCutoff;
	double fConst3;
	FAUSTFLOAT fBandwidth;
	FAUSTFLOAT fPkShGain;
	double fRec1[2];
	double fRec2[2];
	double fRec0[3];
	double fRec3[2];
	double fRec4[2];

 public:

	void metadata(Meta* m) {
	}

	virtual int getNumInputs() {
		return 1;
	}
	virtual int getNumOutputs() {
		return 1;
	}
	virtual int getInputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	virtual int getOutputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}

	static void classInit(int sample_rate) {
	}

	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<double>(192000.0, std::max<double>(1.0, double(fSampleRate)));
		fConst1 = std::exp((0.0 - (1000.0 / fConst0)));
		fConst2 = (6.2831853071795862 / fConst0);
		fConst3 = (2.1775860903036022 / fConst0);
	}

	virtual void instanceResetUserInterface() {
		fCutoff = FAUSTFLOAT(440.0);
		fBandwidth = FAUSTFLOAT(1.0);
		fPkShGain = FAUSTFLOAT(0.0);
	}

	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec1[l0] = 0.0;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec2[l1] = 0.0;
		}
		for (int l2 = 0; (l2 < 3); l2 = (l2 + 1)) {
			fRec0[l2] = 0.0;
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec3[l3] = 0.0;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec4[l4] = 0.0;
		}
	}

	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}

	virtual faustEqPeak* clone() {
		return new faustEqPeak();
	}

	virtual int getSampleRate() {
		return fSampleRate;
	}

	virtual void buildUserInterface(UI* ui_interface) {
	}

	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		double fSlow0 = (fSmoothEnable ? fConst1 : 0.0);
		double fSlow1 = std::max<double>(0.0, double(fCutoff));
		double fSlow2 = (fConst2 * fSlow1);
		double fSlow3 = std::sin(fSlow2);
		double fSlow4 = std::max<double>(0.001, (0.5 / double(sinh(double((fConst3 * ((fSlow1 * double(fBandwidth)) / fSlow3)))))));
		double fSlow5 = std::pow(10.0, (0.025000000000000001 * double(fPkShGain)));
		double fSlow6 = (0.5 * (fSlow3 / (fSlow4 * fSlow5)));
		double fSlow7 = (fSlow6 + 1.0);
		double fSlow8 = (1.0 - fSlow0);
		double fSlow9 = (((0.0 - (2.0 * std::cos(fSlow2))) / fSlow7) * fSlow8);
		double fSlow10 = (((1.0 - fSlow6) / fSlow7) * fSlow8);
		double fSlow11 = (0.5 * ((fSlow3 * fSlow5) / fSlow4));
		double fSlow12 = (((fSlow11 + 1.0) / fSlow7) * fSlow8);
		double fSlow13 = (((1.0 - fSlow11) / fSlow7) * fSlow8);
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			fRec1[0] = ((fSlow0 * fRec1[1]) + fSlow9);
			double fTemp1 = (fRec1[0] * fRec0[1]);
			fRec2[0] = ((fSlow0 * fRec2[1]) + fSlow10);
			fRec0[0] = (fTemp0 - (fTemp1 + (fRec2[0] * fRec0[2])));
			fRec3[0] = ((fSlow0 * fRec3[1]) + fSlow12);
			fRec4[0] = ((fSlow0 * fRec4[1]) + fSlow13);
			output0[i] = FAUSTFLOAT((((fRec0[0] * fRec3[0]) + fTemp1) + (fRec4[0] * fRec0[2])));
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
		}
	}

};

#endif
