/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faust2chHpf6p_H__
#define  __faust2chHpf6p_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS faust2chHpf6p
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faust2chHpf6p : public dsp {
	
 public:
	
	int fSampleRate;
	double fConst0;
	FAUSTFLOAT fCutoff;
	FAUSTFLOAT fQ;
	double fRec0[2];
	double fRec4[2];
	double fRec5[2];
	double fRec3[3];
	double fRec6[2];
	double fRec2[3];
	double fRec1[3];
	double fRec9[3];
	double fRec8[3];
	double fRec7[3];
	
 public:
	
	void metadata(Meta* m) { 
	}

	 int getNumInputs() {
		return 2;
	}
	 int getNumOutputs() {
		return 2;
	}
	 int getInputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
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
	 int getOutputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			case 1: {
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
	
	 void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = (6.2831853071795862 / std::min<double>(192000.0, std::max<double>(1.0, double(fSampleRate))));
	}
	
	 void instanceResetUserInterface() {
		fCutoff = FAUSTFLOAT(440.0);
		fQ = FAUSTFLOAT(0.0);
	}
	
	 void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec4[l1] = 0.0;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec5[l2] = 0.0;
		}
		for (int l3 = 0; (l3 < 3); l3 = (l3 + 1)) {
			fRec3[l3] = 0.0;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec6[l4] = 0.0;
		}
		for (int l5 = 0; (l5 < 3); l5 = (l5 + 1)) {
			fRec2[l5] = 0.0;
		}
		for (int l6 = 0; (l6 < 3); l6 = (l6 + 1)) {
			fRec1[l6] = 0.0;
		}
		for (int l7 = 0; (l7 < 3); l7 = (l7 + 1)) {
			fRec9[l7] = 0.0;
		}
		for (int l8 = 0; (l8 < 3); l8 = (l8 + 1)) {
			fRec8[l8] = 0.0;
		}
		for (int l9 = 0; (l9 < 3); l9 = (l9 + 1)) {
			fRec7[l9] = 0.0;
		}
	}
	
	 void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	 void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	 faust2chHpf6p* clone() {
		return new faust2chHpf6p();
	}
	
	 int getSampleRate() {
		return fSampleRate;
	}
	
	 void buildUserInterface(UI* ui_interface) {
	}
	
	 void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		double fSlow0 = (fConst0 * std::max<double>(0.0, double(fCutoff)));
		double fSlow1 = std::cos(fSlow0);
		double fSlow2 = (0.5 * (std::sin(fSlow0) / std::max<double>(0.001, std::pow(10.0, (0.050000000000000003 * double(fQ))))));
		double fSlow3 = (fSlow2 + 1.0);
		double fSlow4 = (0.0010000000000000009 * ((-1.0 - fSlow1) / fSlow3));
		double fSlow5 = (0.0010000000000000009 * ((0.0 - (2.0 * fSlow1)) / fSlow3));
		double fSlow6 = (0.0010000000000000009 * ((1.0 - fSlow2) / fSlow3));
		double fSlow7 = (0.00050000000000000044 * ((fSlow1 + 1.0) / fSlow3));
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			double fTemp1 = double(input1[i]);
			fRec0[0] = (fSlow4 + (0.999 * fRec0[1]));
			fRec4[0] = (fSlow5 + (0.999 * fRec4[1]));
			fRec5[0] = (fSlow6 + (0.999 * fRec5[1]));
			fRec3[0] = (fTemp0 - ((fRec4[0] * fRec3[1]) + (fRec5[0] * fRec3[2])));
			fRec6[0] = (fSlow7 + (0.999 * fRec6[1]));
			fRec2[0] = (((fRec0[0] * fRec3[1]) + (fRec6[0] * (fRec3[0] + fRec3[2]))) - ((fRec4[0] * fRec2[1]) + (fRec5[0] * fRec2[2])));
			fRec1[0] = (((fRec0[0] * fRec2[1]) + (fRec6[0] * (fRec2[0] + fRec2[2]))) - ((fRec4[0] * fRec1[1]) + (fRec5[0] * fRec1[2])));
			output0[i] = FAUSTFLOAT(((fRec0[0] * fRec1[1]) + (fRec6[0] * (fRec1[0] + fRec1[2]))));
			fRec9[0] = (fTemp1 - ((fRec4[0] * fRec9[1]) + (fRec5[0] * fRec9[2])));
			fRec8[0] = (((fRec0[0] * fRec9[1]) + (fRec6[0] * (fRec9[0] + fRec9[2]))) - ((fRec4[0] * fRec8[1]) + (fRec5[0] * fRec8[2])));
			fRec7[0] = (((fRec0[0] * fRec8[1]) + (fRec6[0] * (fRec8[0] + fRec8[2]))) - ((fRec4[0] * fRec7[1]) + (fRec5[0] * fRec7[2])));
			output1[i] = FAUSTFLOAT(((fRec0[0] * fRec7[1]) + (fRec6[0] * (fRec7[0] + fRec7[2]))));
			fRec0[1] = fRec0[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec6[1] = fRec6[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
		}
	}

};

#endif
