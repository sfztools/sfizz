// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Interpolators.h"
#include "MathHelpers.h"
#include "SIMDConfig.h"

namespace sfz {

template <InterpolatorModel M, class R>
class Interpolator;

template <InterpolatorModel M, class R>
inline R interpolate(const R* values, R coeff)
{
    return Interpolator<M, R>::process(values, coeff);
}

//------------------------------------------------------------------------------
// Linear

template <class R>
class Interpolator<kInterpolatorLinear, R>
{
public:
    static inline R process(const R* values, R coeff)
    {
        return values[0] * (static_cast<R>(1.0) - coeff) + values[1] * coeff;
    }
};

//------------------------------------------------------------------------------
// Hermite 3rd order, SSE specialization

#if SFIZZ_HAVE_SSE
template <>
class Interpolator<kInterpolatorHermite3, float>
{
public:
    static inline float process(const float* values, float coeff)
    {
        __m128 x = _mm_sub_ps(_mm_setr_ps(-1, 0, 1, 2), _mm_set1_ps(coeff));
        __m128 h = hermite3x4(x);
        __m128 y = _mm_mul_ps(h, _mm_loadu_ps(values));
        // sum 4 to 1
        __m128 xmm0 = y;
        __m128 xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0xe5);
        __m128 xmm2 = _mm_movehl_ps(xmm0, xmm0);
        xmm1 = _mm_add_ss(xmm1, xmm0);
        xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0xe7);
        xmm2 = _mm_add_ss(xmm2, xmm1);
        xmm0 = _mm_add_ss(xmm0, xmm2);
        return _mm_cvtss_f32(xmm0);
    }
};
#endif

//------------------------------------------------------------------------------
// Hermite 3rd order, generic

template <class R>
class Interpolator<kInterpolatorHermite3, R>
{
public:
    static inline R process(const R* values, R coeff)
    {
        R y = 0;
        for (int i = 0; i < 4; ++i) {
            R h = hermite3<R>(i - 1 - coeff);
            y += h * values[i];
        }
        return y;
    }
};

//------------------------------------------------------------------------------
// B-spline 3rd order, SSE specialization

#if SFIZZ_HAVE_SSE
template <>
class Interpolator<kInterpolatorBspline3, float>
{
public:
    static inline float process(const float* values, float coeff)
    {
        __m128 x = _mm_sub_ps(_mm_setr_ps(-1, 0, 1, 2), _mm_set1_ps(coeff));
        __m128 h = bspline3x4(x);
        __m128 y = _mm_mul_ps(h, _mm_loadu_ps(values));
        // sum 4 to 1
        __m128 xmm0 = y;
        __m128 xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0xe5);
        __m128 xmm2 = _mm_movehl_ps(xmm0, xmm0);
        xmm1 = _mm_add_ss(xmm1, xmm0);
        xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0xe7);
        xmm2 = _mm_add_ss(xmm2, xmm1);
        xmm0 = _mm_add_ss(xmm0, xmm2);
        return _mm_cvtss_f32(xmm0);
    }
};
#endif

//------------------------------------------------------------------------------
// B-spline 3rd order, generic

template <class R>
class Interpolator<kInterpolatorBspline3, R>
{
public:
    static inline R process(const R* values, R coeff)
    {
        R y = 0;
        for (int i = 0; i < 4; ++i) {
            R h = bspline3<R>(i - 1 - coeff);
            y += h * values[i];
        }
        return y;
    }
};

} // namespace sfz
