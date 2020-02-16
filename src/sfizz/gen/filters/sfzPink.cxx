/* ------------------------------------------------------------
author: "Jean Pierre Cimalando"
license: "BSD-2-Clause"
name: "sfz_filters"
Code generated with Faust 2.15.11 (https://faust.grame.fr)
Compilation options: -inpl -double -ftz 0
------------------------------------------------------------ */

#ifndef  __faustPink_H__
#define  __faustPink_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>


#ifndef FAUSTCLASS 
#define FAUSTCLASS faustPink
#endif
#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustPink : public sfzFilterDsp {
	
 public:
	
	double fRec0[4];
	int fSamplingFreq;
	
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
		switch (channel) {
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
		switch (channel) {
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
	
	static void classInit(int samplingFreq) {
		
	}
	
	virtual void instanceConstants(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		
	}
	
	virtual void instanceResetUserInterface() {
		
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; (l0 < 4); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0;
			
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
	
	virtual faustPink* clone() {
		return new faustPink();
	}
	
	virtual int getSampleRate() {
		return fSamplingFreq;
		
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		
	}
	
	virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		for (int i = 0; (i < count); i = (i + 1)) {
			double fTemp0 = double(input0[i]);
			fRec0[0] = ((((2.4949560019999999 * fRec0[1]) + (0.52218940000000003 * fRec0[3])) + fTemp0) - (2.0172658750000001 * fRec0[2]));
			output0[i] = FAUSTFLOAT((((0.049922034999999997 * fRec0[0]) + (0.050612698999999997 * fRec0[2])) - ((0.095993537000000004 * fRec0[1]) + (0.0044087859999999996 * fRec0[3]))));
			for (int j0 = 3; (j0 > 0); j0 = (j0 - 1)) {
				fRec0[j0] = fRec0[(j0 - 1)];
				
			}
			
		}
		
	}

};

#endif
