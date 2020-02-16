/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.15.11 (https://faust.grame.fr)
Compilation options: -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faust2chEq_H__
#define  __faust2chEq_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS faust2chEq
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faust2chEq : public sfzFilterDsp {
	
 public:
	
	int fSamplingFreq;
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	FAUSTFLOAT fCutoff;
	double fConst4;
	FAUSTFLOAT fBandwidth;
	FAUSTFLOAT fPkShGain;
	double fRec1[2];
	double fRec2[2];
	double fRec0[3];
	double fRec3[2];
	double fRec4[2];
	double fRec5[3];
	
 public:
	
	void metadata(Meta* m) { 
	}

	virtual int getNumInputs() {
		return 2;
		
	}
	virtual int getNumOutputs() {
		return 2;
		
	}
	virtual int getInputRate(int channel) {
		int rate;
		switch (channel) {
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
	virtual int getOutputRate(int channel) {
		int rate;
		switch (channel) {
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
	
	static void classInit(int samplingFreq) {
		
	}
	
	virtual void instanceConstants(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fConst0 = std::min<double>(192000.0, std::max<double>(1.0, double(fSamplingFreq)));
		fConst1 = std::exp((0.0 - (1000.0 / fConst0)));
		fConst2 = (1.0 - fConst1);
		fConst3 = (6.2831853071795862 / fConst0);
		fConst4 = (2.1775860903036022 / fConst0);
		
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
		for (int l5 = 0; (l5 < 3); l5 = (l5 + 1)) {
			fRec5[l5] = 0.0;
			
		}
		
	}
	
	virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	
	virtual void instanceInit(int samplingFreq) {
		instanceConstants(samplingFreq);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual faust2chEq* clone() {
		return new faust2chEq();
	}
	
	virtual int getSampleRate() {
		return fSamplingFreq;
		
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		double fSlow0 = std::max<double>(0.0, double(fCutoff));
		double fSlow1 = (fConst3 * fSlow0);
		double fSlow2 = std::sin(fSlow1);
		double fSlow3 = std::max<double>(0.001, (0.5 / double(sinh(double((fConst4 * ((fSlow0 * double(fBandwidth)) / fSlow2)))))));
		double fSlow4 = std::pow(10.0, (0.025000000000000001 * double(fPkShGain)));
		double fSlow5 = (0.5 * (fSlow2 / (fSlow3 * fSlow4)));
		double fSlow6 = (fSlow5 + 1.0);
		double fSlow7 = (fConst2 * ((0.0 - (2.0 * std::cos(fSlow1))) / fSlow6));
		double fSlow8 = (fConst2 * ((1.0 - fSlow5) / fSlow6));
		double fSlow9 = (0.5 * ((fSlow2 * fSlow4) / fSlow3));
		double fSlow10 = (fConst2 * ((fSlow9 + 1.0) / fSlow6));
		double fSlow11 = (fConst2 * ((1.0 - fSlow9) / fSlow6));
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			double fTemp1 = double(input1[i]);
			fRec1[0] = (fSlow7 + (fConst1 * fRec1[1]));
			double fTemp2 = (fRec1[0] * fRec0[1]);
			fRec2[0] = (fSlow8 + (fConst1 * fRec2[1]));
			fRec0[0] = (fTemp0 - (fTemp2 + (fRec2[0] * fRec0[2])));
			fRec3[0] = (fSlow10 + (fConst1 * fRec3[1]));
			fRec4[0] = (fSlow11 + (fConst1 * fRec4[1]));
			output0[i] = FAUSTFLOAT((((fRec0[0] * fRec3[0]) + fTemp2) + (fRec4[0] * fRec0[2])));
			double fTemp3 = (fRec1[0] * fRec5[1]);
			fRec5[0] = (fTemp1 - ((fRec2[0] * fRec5[2]) + fTemp3));
			output1[i] = FAUSTFLOAT(((fTemp3 + (fRec3[0] * fRec5[0])) + (fRec4[0] * fRec5[2])));
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			
		}
		
	}

};

#endif
