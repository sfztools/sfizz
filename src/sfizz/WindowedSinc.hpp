// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "WindowedSinc.h"
#include <cstdint>

namespace sfz {

template <class T>
inline void AbstractWindowedSinc<T>::fillTable() noexcept
{
    float* table = const_cast<float*>(static_cast<T*>(this)->getTablePointer());
    size_t points = static_cast<T*>(this)->getNumPoints();
    size_t tableSize = static_cast<T*>(this)->getTableSize();

    WindowedSincDetail::calculateTable(
        absl::MakeSpan(table, tableSize), points, beta_, TableExtra);
}

template <class T>
inline float AbstractWindowedSinc<T>::getUnchecked(float x) const noexcept
{
    const float* table = static_cast<const T*>(this)->getTablePointer();
    size_t points = static_cast<const T*>(this)->getNumPoints();
    size_t tableSize = static_cast<const T*>(this)->getTableSize();

    float ix = (x + points / 2.0f) * ((tableSize - 1) / points);
    intptr_t i0 = static_cast<intptr_t>(ix);
    float mu = ix - i0;
    float y0 = table[i0];
    float dy = table[i0 + 1] - y0;
    return y0 + mu * dy;
}

#if SFIZZ_HAVE_SSE2
template <class T>
inline __m128 AbstractWindowedSinc<T>::getUncheckedX4(__m128 x) const noexcept
{
    const float* table = static_cast<const T*>(this)->getTablePointer();
    size_t points = static_cast<const T*>(this)->getNumPoints();
    size_t tableSize = static_cast<const T*>(this)->getTableSize();

    __m128 ix = _mm_mul_ps(
        _mm_add_ps(x, _mm_set1_ps(points / 2.0f)),
        _mm_set1_ps((tableSize - 1) / points));
    alignas(__m128i) int j0[4];
    __m128i i0 = _mm_cvttps_epi32(ix);
    _mm_store_si128((__m128i*)j0, i0);
    __m128 mu = _mm_sub_ps(ix, _mm_cvtepi32_ps(i0));

    // reference: Interpolated table lookups using SSE2 [2/2]
    // https://rawstudio.org/blog/?p=482
    __m128 p0p1 = _mm_castsi128_ps(_mm_loadl_epi64((__m128i*)&table[j0[0]]));
    __m128 p2p3 = _mm_castsi128_ps(_mm_loadl_epi64((__m128i*)&table[j0[2]]));
    p0p1 = _mm_loadh_pi(p0p1, (__m64*)&table[j0[1]]);
    p2p3 = _mm_loadh_pi(p2p3, (__m64*)&table[j0[3]]);
    __m128 y0 = _mm_shuffle_ps(p0p1, p2p3, _MM_SHUFFLE(2, 0, 2, 0));
    __m128 y1 = _mm_shuffle_ps(p0p1, p2p3, _MM_SHUFFLE(3, 1, 3, 1));

    __m128 dy = _mm_sub_ps(y1, y0);
    return _mm_add_ps(y0, _mm_mul_ps(mu, dy));
}
#endif

template <class T>
inline double AbstractWindowedSinc<T>::getExact(double x) const noexcept
{
    size_t points = static_cast<const T*>(this)->getNumPoints();
    return WindowedSincDetail::calculateExact(x, points, beta_);
}

} // namespace sfz
