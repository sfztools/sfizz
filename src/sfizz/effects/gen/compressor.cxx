/* ------------------------------------------------------------
name: "compressor"
Code generated with Faust 2.27.2 (https://faust.grame.fr)
Compilation options: -lang cpp -inpl -scal -ftz 0
------------------------------------------------------------ */

#ifndef  __faustCompressor_H__
#define  __faustCompressor_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <math.h>


#ifndef FAUSTCLASS
#define FAUSTCLASS faustCompressor
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

class faustCompressor {

 public:

	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	int fSampleRate;
	float fConst2;
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT fHslider2;
	float fRec2[2];
	float fRec1[2];
	FAUSTFLOAT fHslider3;
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
		fConst1 = (0.5f * fConst0);
		fConst2 = (1.0f / std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate))));
	}

	void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.0f);
		fHslider1 = FAUSTFLOAT(1.0f);
		fHslider2 = FAUSTFLOAT(0.0f);
		fHslider3 = FAUSTFLOAT(0.0f);
	}

	void instanceClear() {
		for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
			fRec2[l0] = 0.0f;
		}
		for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
			fRec1[l1] = 0.0f;
		}
		for (int l2 = 0; (l2 < 2); l2 = (l2 + 1)) {
			fRec0[l2] = 0.0f;
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

	faustCompressor* clone() {
		return new faustCompressor();
	}

	int getSampleRate() {
		return fSampleRate;
	}

	void buildUserInterface() {
	}

	void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = float(fHslider0);
		float fSlow1 = (fConst1 * fSlow0);
		int iSlow2 = (std::fabs(fSlow1) < 1.1920929e-07f);
		float fSlow3 = (iSlow2 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow2 ? 1.0f : fSlow1)))));
		float fSlow4 = ((1.0f / std::max<float>(1.00000001e-07f, float(fHslider1))) + -1.0f);
		float fSlow5 = (fConst0 * fSlow0);
		int iSlow6 = (std::fabs(fSlow5) < 1.1920929e-07f);
		float fSlow7 = (iSlow6 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow6 ? 1.0f : fSlow5)))));
		float fSlow8 = (fConst0 * float(fHslider2));
		int iSlow9 = (std::fabs(fSlow8) < 1.1920929e-07f);
		float fSlow10 = (iSlow9 ? 0.0f : std::exp((0.0f - (fConst2 / (iSlow9 ? 1.0f : fSlow8)))));
		float fSlow11 = float(fHslider3);
		float fSlow12 = (1.0f - fSlow3);
		for (int i = 0; (i < count); i = (i + 1)) {
			float fTemp0 = float(input0[i]);
			float fTemp1 = std::fabs(fTemp0);
			float fTemp2 = ((fRec1[1] > fTemp1) ? fSlow10 : fSlow7);
			fRec2[0] = ((fRec2[1] * fTemp2) + (fTemp1 * (1.0f - fTemp2)));
			fRec1[0] = fRec2[0];
			fRec0[0] = ((fRec0[1] * fSlow3) + (fSlow4 * (std::max<float>(((20.0f * std::log10(fRec1[0])) - fSlow11), 0.0f) * fSlow12)));
			output0[i] = FAUSTFLOAT(std::pow(10.0f, (0.0500000007f * fRec0[0])));
			fRec2[1] = fRec2[0];
			fRec1[1] = fRec1[0];
			fRec0[1] = fRec0[0];
		}
	}

};

#ifdef FAUST_UIMACROS



	#define FAUST_LIST_ACTIVES(p) \
		p(HORIZONTALSLIDER, Ratio, "Ratio", fHslider1, 1.0f, 1.0f, 20.0f, 0.01f) \
		p(HORIZONTALSLIDER, Threshold, "Threshold", fHslider3, 0.0f, -60.0f, 0.0f, 0.01f) \
		p(HORIZONTALSLIDER, Attack, "Attack", fHslider0, 0.0f, 0.0f, 0.5f, 0.001f) \
		p(HORIZONTALSLIDER, Release, "Release", fHslider2, 0.0f, 0.0f, 5.0f, 0.001f) \

	#define FAUST_LIST_PASSIVES(p) \

#endif

#endif
