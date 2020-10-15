/* ------------------------------------------------------------
name: "gate"
Code generated with Faust 2.27.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __faustGate_H__
#define  __faustGate_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS
#define FAUSTCLASS faustGate
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustGate {

 public:

	float fConst0;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	int fSampleRate;
	float fConst1;
	float fConst2;
	float fRec3[2];
	FAUSTFLOAT fHslider2;
	int iVec0[2];
	float fConst3;
	FAUSTFLOAT fHslider3;
	int iRec4[2];
	float fRec1[2];
	float fRec0[2];

 public:

	void metadata() {
	}

	int getNumInputs() {
		return 1;
	}
	int getNumOutputs() {
		return 1;
	}
	int getInputRate(int channel) {
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
	int getOutputRate(int channel) {
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
		(void)sample_rate;
	}

	void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = float(_oversampling);
		fConst1 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst2 = (1.0f / fConst1);
		fConst3 = (fConst1 * fConst0);
	}

	void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.0f);
		fHslider1 = FAUSTFLOAT(0.0f);
		fHslider2 = FAUSTFLOAT(0.0f);
		fHslider3 = FAUSTFLOAT(0.0f);
	}

	void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec3[l0] = 0.0f;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			iVec0[l1] = 0;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			iRec4[l2] = 0;
		}
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			fRec1[l3] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fRec0[l4] = 0.0f;
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

	faustGate* clone() {
		return new faustGate();
	}

	int getSampleRate() {
		return fSampleRate;
	}

	void buildUserInterface() {
	}

	void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = (fConst0 * float(fHslider0));
		float fSlow1 = (fConst0 * float(fHslider1));
		float fSlow2 = std::min<float>(fSlow0, fSlow1);
		int iSlow3 = (std::fabs(fSlow2) < 1.1920929e-07f);
		float fSlow4 = (iSlow3 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow3 ? 1.0f : fSlow2)))));
		float fSlow5 = (1.0f - fSlow4);
		float fSlow6 = std::pow(10.0f, (0.0500000007f * float(fHslider2)));
		int iSlow7 = int((fConst3 * float(fHslider3)));
		int iSlow8 = (std::fabs(fSlow0) < 1.1920929e-07f);
		float fSlow9 = (iSlow8 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow8 ? 1.0f : fSlow0)))));
		int iSlow10 = (std::fabs(fSlow1) < 1.1920929e-07f);
		float fSlow11 = (iSlow10 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow10 ? 1.0f : fSlow1)))));
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			fRec3[0] = ((fRec3[1] * fSlow4) + (std::fabs(fTemp0) * fSlow5));
			float fRec2 = fRec3[0];
			int iTemp1 = (fRec2 > fSlow6);
			iVec0[0] = iTemp1;
			iRec4[0] = std::max<int>(int((iSlow7 * (iTemp1 < iVec0[1]))), int((iRec4[1] + -1)));
			float fTemp2 = std::fabs(std::max<float>(float(iTemp1), float((iRec4[0] > 0))));
			float fTemp3 = ((fRec0[1] > fTemp2) ? fSlow11 : fSlow9);
			fRec1[0] = ((fRec1[1] * fTemp3) + (fTemp2 * (1.0f - fTemp3)));
			fRec0[0] = fRec1[0];
			output0[i] = FAUSTFLOAT(fRec0[0]);
			fRec3[1] = fRec3[0];
			iVec0[1] = iVec0[0];
			iRec4[1] = iRec4[0];
			fRec1[1] = fRec1[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#ifdef FAUST_UIMACROS



	#define FAUST_LIST_ACTIVES(p) \
		p(HORIZONTALSLIDER, Threshold, "Threshold", fHslider2, 0.0f, -60.0f, 0.0f, 0.01f) \
		p(HORIZONTALSLIDER, Attack, "Attack", fHslider0, 0.0f, 0.0f, 10.0f, 0.001f) \
		p(HORIZONTALSLIDER, Hold, "Hold", fHslider3, 0.0f, 0.0f, 10.0f, 0.001f) \
		p(HORIZONTALSLIDER, Release, "Release", fHslider1, 0.0f, 0.0f, 5.0f, 0.001f) \

	#define FAUST_LIST_PASSIVES(p) \

#endif

#endif
