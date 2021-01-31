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

template <class T>
inline double AbstractWindowedSinc<T>::getExact(double x) const noexcept
{
    size_t points = static_cast<const T*>(this)->getNumPoints();
    return WindowedSincDetail::calculateExact(x, points, beta_);
}

} // namespace sfz
