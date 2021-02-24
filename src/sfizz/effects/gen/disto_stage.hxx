/* ------------------------------------------------------------
name: "disto_stage"
Code generated with Faust 2.27.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __faustDisto_H__
#define  __faustDisto_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>

class faustDistoSIG0 {

  public:

	int iRec3[2];

  public:

	int getNumInputsfaustDistoSIG0() {
		return 0;
	}
	int getNumOutputsfaustDistoSIG0() {
		return 1;
	}
	int getInputRatefaustDistoSIG0(int channel) {
		int rate;
		switch ((channel)) {
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	int getOutputRatefaustDistoSIG0(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 0;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}

	void instanceInitfaustDistoSIG0(int sample_rate) {
		(void)sample_rate;
		for (int l3 = 0; (l3 < 2); l3 = (l3 + 1)) {
			iRec3[l3] = 0;
		}
	}

	void fillfaustDistoSIG0(int count, float* table) {
		for (int i = 0; (i < count); i = (i + 1)) {
			iRec3[0] = (iRec3[1] + 1);
			float fTemp1 = std::exp(((0.078125f * float((iRec3[0] + -1))) + -10.0f));
			table[i] = (fTemp1 / (fTemp1 + 1.0f));
			iRec3[1] = iRec3[0];
		}
	}

};

static faustDistoSIG0* newfaustDistoSIG0() { return (faustDistoSIG0*)new faustDistoSIG0(); }
static void deletefaustDistoSIG0(faustDistoSIG0* dsp) { delete dsp; }

static float ftbl0faustDistoSIG0[256];

#ifndef FAUSTCLASS
#define FAUSTCLASS faustDisto
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustDisto {

 public:

	float fVec0[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	float fConst3;
	float fConst4;
	int iConst5;
	float fConst6;
	int iRec2[2];
	float fConst7;
	float fRec1[2];
	FAUSTFLOAT fHslider0;
	float fVec1[2];
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
		faustDistoSIG0* sig0 = newfaustDistoSIG0();
		sig0->instanceInitfaustDistoSIG0(sample_rate);
		sig0->fillfaustDistoSIG0(256, ftbl0faustDistoSIG0);
		deletefaustDistoSIG0(sig0);
	}

	void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = (15.707963f / fConst0);
		fConst2 = (1.0f / (fConst1 + 1.0f));
		fConst3 = (1.0f - fConst1);
		fConst4 = (0.00999999978f * float(_oversampling));
		iConst5 = (std::fabs(fConst4) < 1.1920929e-07f);
		fConst6 = (iConst5 ? 0.0f : std::exp((0.0f - ((1.0f / fConst0) / (iConst5 ? 1.0f : fConst4)))));
		fConst7 = (1.0f - fConst6);
	}

	void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(100.0f);
	}

	void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fVec0[l0] = 0.0f;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			iRec2[l1] = 0;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec1[l2] = 0.0f;
		}
		for (int l4 = 0; (l4 < 2); l4 = (l4 + 1)) {
			fVec1[l4] = 0.0f;
		}
		for (int l5 = 0; (l5 < 2); l5 = (l5 + 1)) {
			fRec0[l5] = 0.0f;
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

	faustDisto* clone() {
		return new faustDisto();
	}

	int getSampleRate() {
		return fSampleRate;
	}

	void buildUserInterface() {
	}

	void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = ((0.200000003f * float(fHslider0)) + 2.0f);
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			fVec0[0] = fTemp0;
			iRec2[0] = (((fTemp0 < fVec0[1]) & (fTemp0 < -0.25f)) ? 1 : (((fTemp0 > fVec0[1]) & (fTemp0 > 0.25f)) ? 0 : iRec2[1]));
			fRec1[0] = ((fRec1[1] * fConst6) + (float(iRec2[0]) * fConst7));
			float fTemp2 = std::max<float>(0.0f, (12.75f * ((fSlow0 * fTemp0) + 10.0f)));
			int iTemp3 = int(fTemp2);
			float fTemp4 = ftbl0faustDistoSIG0[std::min<int>(255, iTemp3)];
			float fTemp5 = (fTemp4 + ((fTemp2 - float(iTemp3)) * (ftbl0faustDistoSIG0[std::min<int>(255, (iTemp3 + 1))] - fTemp4)));
			float fTemp6 = ((fRec1[0] * (fTemp5 + -1.0f)) + ((1.0f - fRec1[0]) * fTemp5));
			fVec1[0] = fTemp6;
			fRec0[0] = (fConst2 * ((fConst3 * fRec0[1]) + (2.0f * (fTemp6 - fVec1[1]))));
			output0[i] = FAUSTFLOAT(fRec0[0]);
			fVec0[1] = fVec0[0];
			iRec2[1] = iRec2[0];
			fRec1[1] = fRec1[0];
			fVec1[1] = fVec1[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#ifdef FAUST_UIMACROS



	#define FAUST_LIST_ACTIVES(p) \
		p(HORIZONTALSLIDER, Depth, "Depth", fHslider0, 100.0f, 0.0f, 100.0f, 0.01f) \

	#define FAUST_LIST_PASSIVES(p) \

#endif

#endif
