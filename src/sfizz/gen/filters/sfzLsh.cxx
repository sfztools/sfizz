/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faustLsh_H__
#define  __faustLsh_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS
#define FAUSTCLASS faustLsh
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustLsh : public sfzFilterDsp {

 public:

	int fSampleRate;
	double fConst0;
	double fConst1;
	FAUSTFLOAT fPkShGain;
	double fConst2;
	FAUSTFLOAT fCutoff;
	FAUSTFLOAT fQ;
	double fRec0[2];
	double fRec2[2];
	double fRec3[2];
	double fRec1[3];
	double fRec4[2];
	double fRec5[2];

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
	}

	virtual void instanceResetUserInterface() {
		fPkShGain = FAUSTFLOAT(0.0);
		fCutoff = FAUSTFLOAT(440.0);
		fQ = FAUSTFLOAT(0.0);
	}

	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec2[l1] = 0.0;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec3[l2] = 0.0;
		}
		for (int l3 = 0; (l3 < 3); l3 = (l3 + 1)) {
			fRec1[l3] = 0.0;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec4[l4] = 0.0;
		}
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			fRec5[l5] = 0.0;
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

	virtual faustLsh* clone() {
		return new faustLsh();
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
		double fSlow1 = std::pow(10.0, (0.025000000000000001 * double(fPkShGain)));
		double fSlow2 = (fConst2 * std::max<double>(0.0, double(fCutoff)));
		double fSlow3 = std::cos(fSlow2);
		double fSlow4 = (fSlow3 * (fSlow1 + 1.0));
		double fSlow5 = ((std::sqrt(fSlow1) * std::sin(fSlow2)) / std::max<double>(0.001, std::pow(10.0, (0.050000000000000003 * double(fQ)))));
		double fSlow6 = (fSlow3 * (fSlow1 + -1.0));
		double fSlow7 = (fSlow1 + fSlow6);
		double fSlow8 = ((fSlow5 + fSlow7) + 1.0);
		double fSlow9 = (1.0 - fSlow0);
		double fSlow10 = ((2.0 * ((fSlow1 * (fSlow1 + (-1.0 - fSlow4))) / fSlow8)) * fSlow9);
		double fSlow11 = (((0.0 - (2.0 * ((fSlow1 + fSlow4) + -1.0))) / fSlow8) * fSlow9);
		double fSlow12 = (((fSlow7 + (1.0 - fSlow5)) / fSlow8) * fSlow9);
		double fSlow13 = (((fSlow1 * ((fSlow1 + fSlow5) + (1.0 - fSlow6))) / fSlow8) * fSlow9);
		double fSlow14 = (((fSlow1 * (fSlow1 + (1.0 - (fSlow5 + fSlow6)))) / fSlow8) * fSlow9);
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			fRec0[0] = ((fSlow0 * fRec0[1]) + fSlow10);
			fRec2[0] = ((fSlow0 * fRec2[1]) + fSlow11);
			fRec3[0] = ((fSlow0 * fRec3[1]) + fSlow12);
			fRec1[0] = (fTemp0 - ((fRec2[0] * fRec1[1]) + (fRec3[0] * fRec1[2])));
			fRec4[0] = ((fSlow0 * fRec4[1]) + fSlow13);
			fRec5[0] = ((fSlow0 * fRec5[1]) + fSlow14);
			output0[i] = FAUSTFLOAT(((fRec0[0] * fRec1[1]) + ((fRec1[0] * fRec4[0]) + (fRec5[0] * fRec1[2]))));
			fRec0[1] = fRec0[0];
			fRec2[1] = fRec2[0];
			fRec3[1] = fRec3[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
		}
	}

};

#endif
