/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.20.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faust2chLpf2pSv_H__
#define  __faust2chLpf2pSv_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS faust2chLpf2pSv
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faust2chLpf2pSv : public dsp {
	
 public:
	
	int fSampleRate;
	double fConst0;
	FAUSTFLOAT fCutoff;
	double fRec3[2];
	FAUSTFLOAT fQ;
	double fRec4[2];
	double fRec5[2];
	double fRec1[2];
	double fRec2[2];
	double fRec7[2];
	double fRec8[2];
	
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
		fConst0 = (3.1415926535897931 / std::min<double>(192000.0, std::max<double>(1.0, double(fSampleRate))));
	}
	
	 void instanceResetUserInterface() {
		fCutoff = FAUSTFLOAT(440.0);
		fQ = FAUSTFLOAT(0.0);
	}
	
	 void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec3[l0] = 0.0;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec4[l1] = 0.0;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec5[l2] = 0.0;
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec1[l3] = 0.0;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec2[l4] = 0.0;
		}
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			fRec7[l5] = 0.0;
		}
		for (int l6 = 0; (l6 < 2); l6 = (l6 + 1)) {
			fRec8[l6] = 0.0;
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
	
	 faust2chLpf2pSv* clone() {
		return new faust2chLpf2pSv();
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
		double fSlow0 = (0.0010000000000000009 * std::tan((fConst0 * double(fCutoff))));
		double fSlow1 = (1.0 / std::pow(10.0, (0.050000000000000003 * double(fQ))));
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			double fTemp1 = double(input1[i]);
			fRec3[0] = (fSlow0 + (0.999 * fRec3[1]));
			double fTemp2 = (fSlow1 + fRec3[0]);
			fRec4[0] = ((0.999 * fRec4[1]) + (0.0010000000000000009 / ((fRec3[0] * fTemp2) + 1.0)));
			double fTemp3 = (fRec3[0] * fRec4[0]);
			fRec5[0] = ((0.999 * fRec5[1]) + (0.0010000000000000009 * fTemp2));
			double fTemp4 = (fTemp0 - (fRec1[1] + (fRec5[0] * fRec2[1])));
			double fTemp5 = (fTemp3 * fTemp4);
			double fTemp6 = (fRec2[1] + (2.0 * fTemp5));
			double fRec0 = (fRec1[1] + (fRec3[0] * fTemp6));
			double fTemp7 = (fRec2[1] + fTemp5);
			fRec1[0] = (fRec1[1] + (2.0 * (fRec3[0] * fTemp7)));
			fRec2[0] = fTemp6;
			output0[i] = FAUSTFLOAT(fRec0);
			double fTemp8 = (fTemp1 - (fRec7[1] + (fRec5[0] * fRec8[1])));
			double fTemp9 = (fTemp3 * fTemp8);
			double fTemp10 = (fRec8[1] + (2.0 * fTemp9));
			double fRec6 = (fRec7[1] + (fRec3[0] * fTemp10));
			double fTemp11 = (fRec8[1] + fTemp9);
			fRec7[0] = (fRec7[1] + (2.0 * (fRec3[0] * fTemp11)));
			fRec8[0] = fTemp10;
			output1[i] = FAUSTFLOAT(fRec6);
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
		}
	}

};

#endif
