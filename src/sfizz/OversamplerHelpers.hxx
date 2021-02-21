// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

//------------------------------------------------------------------------------
// This is generated by the Sfizz HIIR designer
// Using options: -a 96 -t 0.01 -o 128
//------------------------------------------------------------------------------

#pragma once
#include "OversamplerHelpers.h"

namespace sfz {

// 2x <-> 1x: TBW = 0.01
static constexpr double OSCoeffs2x[12] = {
	0.036681502163648017,
	0.136547624631957715,
	0.274631759379454110,
	0.423138617436566666,
	0.561098697879194752,
	0.677540049974161618,
	0.769741833863226588,
	0.839889624849638028,
	0.892260818003878908,
	0.931541959963183896,
	0.962094548378083947,
	0.987816370732897076,
};
// 4x <-> 2x: TBW = 0.255
static constexpr double OSCoeffs4x[4] = {
	0.041893991997656171,
	0.168903482439952013,
	0.390560772921165922,
	0.743895748268478152,
};
// 8x <-> 4x: TBW = 0.3775
static constexpr double OSCoeffs8x[3] = {
	0.055748680811302048,
	0.243051195741530918,
	0.646699131192682297,
};
// 16x <-> 8x: TBW = 0.43875
static constexpr double OSCoeffs16x[2] = {
	0.107172166664564611,
	0.530904350331903085,
};
// 32x <-> 16x: TBW = 0.469375
static constexpr double OSCoeffs32x[2] = {
	0.105969237763476387,
	0.528620279623742473,
};
// 64x <-> 32x: TBW = 0.484687
static constexpr double OSCoeffs64x[1] = {
	0.333526281707771211,
};
// 128x <-> 64x: TBW = 0.492344
static constexpr double OSCoeffs128x[1] = {
	0.333381553051105561,
};

class Upsampler {
public:
	Upsampler()
	{
		up2_.set_coefs(OSCoeffs2x);
		up4_.set_coefs(OSCoeffs4x);
		up8_.set_coefs(OSCoeffs8x);
		up16_.set_coefs(OSCoeffs16x);
		up32_.set_coefs(OSCoeffs32x);
		up64_.set_coefs(OSCoeffs64x);
		up128_.set_coefs(OSCoeffs128x);
	}
	void clear()
	{
		up2_.clear_buffers();
		up4_.clear_buffers();
		up8_.clear_buffers();
		up16_.clear_buffers();
		up32_.clear_buffers();
		up64_.clear_buffers();
		up128_.clear_buffers();
	}
	static int recommendedBuffer(int factor, int spl)
	{
		return factor * spl;
	}
	static bool canProcess(int factor)
	{
		switch (factor) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
			return true;
		default:
			return false;
		}
	}
	void process(int factor, const float *in, float *out, int spl, float *temp, int ntemp)
	{
		switch (factor) {
		case 1:
			if (in != out) std::memcpy(out, in, spl * sizeof(float));
			break;
		case 2:
			process2x(in, out, spl, temp, ntemp);
			break;
		case 4:
			process4x(in, out, spl, temp, ntemp);
			break;
		case 8:
			process8x(in, out, spl, temp, ntemp);
			break;
		case 16:
			process16x(in, out, spl, temp, ntemp);
			break;
		case 32:
			process32x(in, out, spl, temp, ntemp);
			break;
		case 64:
			process64x(in, out, spl, temp, ntemp);
			break;
		case 128:
			process128x(in, out, spl, temp, ntemp);
			break;
		default:
			ASSERTFALSE;
			break;
		}
	}
	void process2x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 2;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 1 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(out, in, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process4x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 4;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 2 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(out, t1, 2 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process8x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 8;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 4 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(t2, t1, 2 * curspl);
			up8_.process_block(out, t2, 4 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process16x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 16;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 8 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(t2, t1, 2 * curspl);
			up8_.process_block(t1, t2, 4 * curspl);
			up16_.process_block(out, t1, 8 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process32x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 32;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 16 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(t2, t1, 2 * curspl);
			up8_.process_block(t1, t2, 4 * curspl);
			up16_.process_block(t2, t1, 8 * curspl);
			up32_.process_block(out, t2, 16 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process64x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 64;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 32 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(t2, t1, 2 * curspl);
			up8_.process_block(t1, t2, 4 * curspl);
			up16_.process_block(t2, t1, 8 * curspl);
			up32_.process_block(t1, t2, 16 * curspl);
			up64_.process_block(out, t1, 32 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process128x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 128;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 64 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			up2_.process_block(t1, in, 1 * curspl);
			up4_.process_block(t2, t1, 2 * curspl);
			up8_.process_block(t1, t2, 4 * curspl);
			up16_.process_block(t2, t1, 8 * curspl);
			up32_.process_block(t1, t2, 16 * curspl);
			up64_.process_block(t2, t1, 32 * curspl);
			up128_.process_block(out, t2, 64 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
private:
	hiir::Upsampler2x<12> up2_;
	hiir::Upsampler2x<4> up4_;
	hiir::Upsampler2x<3> up8_;
	hiir::Upsampler2x<2> up16_;
	hiir::Upsampler2x<2> up32_;
	hiir::Upsampler2x<1> up64_;
	hiir::Upsampler2x<1> up128_;
};

class Downsampler {
public:
	Downsampler()
	{
		down128_.set_coefs(OSCoeffs128x);
		down64_.set_coefs(OSCoeffs64x);
		down32_.set_coefs(OSCoeffs32x);
		down16_.set_coefs(OSCoeffs16x);
		down8_.set_coefs(OSCoeffs8x);
		down4_.set_coefs(OSCoeffs4x);
		down2_.set_coefs(OSCoeffs2x);
	}
	void clear()
	{
		down128_.clear_buffers();
		down64_.clear_buffers();
		down32_.clear_buffers();
		down16_.clear_buffers();
		down8_.clear_buffers();
		down4_.clear_buffers();
		down2_.clear_buffers();
	}
	static int recommendedBuffer(int factor, int spl)
	{
		return factor * spl;
	}
	static bool canProcess(int factor)
	{
		switch (factor) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
			return true;
		default:
			return false;
		}
	}
	void process(int factor, const float *in, float *out, int spl, float *temp, int ntemp)
	{
		switch (factor) {
		case 128:
			process128x(in, out, spl, temp, ntemp);
			break;
		case 64:
			process64x(in, out, spl, temp, ntemp);
			break;
		case 32:
			process32x(in, out, spl, temp, ntemp);
			break;
		case 16:
			process16x(in, out, spl, temp, ntemp);
			break;
		case 8:
			process8x(in, out, spl, temp, ntemp);
			break;
		case 4:
			process4x(in, out, spl, temp, ntemp);
			break;
		case 2:
			process2x(in, out, spl, temp, ntemp);
			break;
		case 1:
			if (in != out) std::memcpy(out, in, spl * sizeof(float));
			break;
		default:
			ASSERTFALSE;
			break;
		}
	}
	void process2x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 2;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 1 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down2_.process_block(out, in, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process4x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 4;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 2 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down4_.process_block(t1, in, 2 * curspl);
			down2_.process_block(out, t1, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process8x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 8;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 4 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down8_.process_block(t1, in, 4 * curspl);
			down4_.process_block(t2, t1, 2 * curspl);
			down2_.process_block(out, t2, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process16x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 16;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 8 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down16_.process_block(t1, in, 8 * curspl);
			down8_.process_block(t2, t1, 4 * curspl);
			down4_.process_block(t1, t2, 2 * curspl);
			down2_.process_block(out, t1, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process32x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 32;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 16 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down32_.process_block(t1, in, 16 * curspl);
			down16_.process_block(t2, t1, 8 * curspl);
			down8_.process_block(t1, t2, 4 * curspl);
			down4_.process_block(t2, t1, 2 * curspl);
			down2_.process_block(out, t2, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process64x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 64;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 32 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down64_.process_block(t1, in, 32 * curspl);
			down32_.process_block(t2, t1, 16 * curspl);
			down16_.process_block(t1, t2, 8 * curspl);
			down8_.process_block(t2, t1, 4 * curspl);
			down4_.process_block(t1, t2, 2 * curspl);
			down2_.process_block(out, t1, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
	void process128x(const float *in, float *out, int spl, float *temp, int ntemp)
	{
		int maxspl = ntemp / 128;
		ASSERT(maxspl >= 0);
		float *t1 = temp;
		float *t2 = temp + 64 * maxspl;
		(void)t1;
		(void)t2;
		while (spl > 0) {
			int curspl = (spl < maxspl) ? spl : maxspl;
			down128_.process_block(t1, in, 64 * curspl);
			down64_.process_block(t2, t1, 32 * curspl);
			down32_.process_block(t1, t2, 16 * curspl);
			down16_.process_block(t2, t1, 8 * curspl);
			down8_.process_block(t1, t2, 4 * curspl);
			down4_.process_block(t2, t1, 2 * curspl);
			down2_.process_block(out, t2, 1 * curspl);
			in += curspl;
			out += curspl;
			spl -= curspl;
		}
	}
private:
	hiir::Downsampler2x<1> down128_;
	hiir::Downsampler2x<1> down64_;
	hiir::Downsampler2x<2> down32_;
	hiir::Downsampler2x<2> down16_;
	hiir::Downsampler2x<3> down8_;
	hiir::Downsampler2x<4> down4_;
	hiir::Downsampler2x<12> down2_;
};

} // namespace sfz
