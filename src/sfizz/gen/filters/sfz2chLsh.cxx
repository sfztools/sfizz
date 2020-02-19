/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.15.11 (https://faust.grame.fr)
Compilation options: -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faust2chLsh_H__
#define  __faust2chLsh_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS 
#define FAUSTCLASS faust2chLsh
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faust2chLsh : public sfzFilterDsp {
	
 public:
	
	int fSamplingFreq;
	double fConst0;
	double fConst1;
	FAUSTFLOAT fPkShGain;
	double fConst2;
	FAUSTFLOAT fCutoff;
	FAUSTFLOAT fQ;
	double fRec1[2];
	double fRec2[2];
	double fRec0[3];
	double fRec3[2];
	double fRec4[2];
	double fRec5[2];
	double fRec6[3];
	
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
		fConst2 = (6.2831853071795862 / fConst0);
		
	}
	
	virtual void instanceResetUserInterface() {
		fPkShGain = FAUSTFLOAT(0.0);
		fCutoff = FAUSTFLOAT(440.0);
		fQ = FAUSTFLOAT(0.0);
		
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
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			fRec5[l5] = 0.0;
			
		}
		for (int l6 = 0; (l6 < 3); l6 = (l6 + 1)) {
			fRec6[l6] = 0.0;
			
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
	
	virtual faust2chLsh* clone() {
		return new faust2chLsh();
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
		double fSlow0 = (fSmoothEnable?fConst1:0.0);
		double fSlow1 = std::pow(10.0, (0.025000000000000001 * double(fPkShGain)));
		double fSlow2 = (fConst2 * std::max<double>(0.0, double(fCutoff)));
		double fSlow3 = std::cos(fSlow2);
		double fSlow4 = ((fSlow1 + 1.0) * fSlow3);
		double fSlow5 = ((fSlow1 + -1.0) * fSlow3);
		double fSlow6 = (fSlow5 + fSlow1);
		double fSlow7 = ((std::sqrt(fSlow1) * std::sin(fSlow2)) / std::max<double>(0.001, std::pow(10.0, (0.050000000000000003 * double(fQ)))));
		double fSlow8 = ((fSlow6 + fSlow7) + 1.0);
		double fSlow9 = (1.0 - fSlow0);
		double fSlow10 = (((0.0 - (2.0 * ((fSlow4 + fSlow1) + -1.0))) / fSlow8) * fSlow9);
		double fSlow11 = (((fSlow6 + (1.0 - fSlow7)) / fSlow8) * fSlow9);
		double fSlow12 = (((((fSlow1 + fSlow7) + (1.0 - fSlow5)) * fSlow1) / fSlow8) * fSlow9);
		double fSlow13 = ((2.0 * (((fSlow1 + (-1.0 - fSlow4)) * fSlow1) / fSlow8)) * fSlow9);
		double fSlow14 = ((((fSlow1 + (1.0 - (fSlow5 + fSlow7))) * fSlow1) / fSlow8) * fSlow9);
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			double fTemp1 = double(input1[i]);
			fRec1[0] = ((fRec1[1] * fSlow0) + fSlow10);
			fRec2[0] = ((fRec2[1] * fSlow0) + fSlow11);
			fRec0[0] = (fTemp0 - ((fRec1[0] * fRec0[1]) + (fRec2[0] * fRec0[2])));
			fRec3[0] = ((fRec3[1] * fSlow0) + fSlow12);
			fRec4[0] = ((fRec4[1] * fSlow0) + fSlow13);
			fRec5[0] = ((fRec5[1] * fSlow0) + fSlow14);
			output0[i] = FAUSTFLOAT((((fRec0[0] * fRec3[0]) + (fRec4[0] * fRec0[1])) + (fRec5[0] * fRec0[2])));
			fRec6[0] = (fTemp1 - ((fRec1[0] * fRec6[1]) + (fRec2[0] * fRec6[2])));
			output1[i] = FAUSTFLOAT((((fRec3[0] * fRec6[0]) + (fRec4[0] * fRec6[1])) + (fRec5[0] * fRec6[2])));
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec3[1] = fRec3[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			
		}
		
	}

};

#endif
