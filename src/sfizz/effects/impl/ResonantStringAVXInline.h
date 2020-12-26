// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ResonantStringAVX.h"

namespace sfz {
namespace fx {

inline __m256 ResonantStringAVX::process(__m256 input)
{
    fRec0[0] = _mm256_mul_ps(fControl[1], _mm256_add_ps(_mm256_mul_ps(fControl[4], fRec1[1]), _mm256_mul_ps(fControl[5], fRec0[1])));
    __m256 fTemp0 = input;
    fRec2[0] = _mm256_sub_ps(fTemp0, _mm256_mul_ps(fControl[15], _mm256_add_ps(_mm256_mul_ps(fControl[16], fRec2[1]), _mm256_mul_ps(fControl[17], fRec2[2]))));
    fRec1[0] = _mm256_sub_ps(_mm256_add_ps(_mm256_mul_ps(fControl[14], fRec2[2]), _mm256_add_ps(_mm256_mul_ps(fControl[5], fRec1[1]), _mm256_mul_ps(fControl[13], fRec2[0]))),_mm256_mul_ps(fControl[4], fRec0[1]));
    __m256 output = _mm256_mul_ps(fControl[0], fRec0[0]);
    fRec0[1] = fRec0[0];
    fRec2[2] = fRec2[1];
    fRec2[1] = fRec2[0];
    fRec1[1] = fRec1[0];
    return output;
}

} // namespace sfz
} // namespace fx
