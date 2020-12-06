// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ResonantString.h"

namespace sfz {
namespace fx {

inline float ResonantString::process(float input)
{
    fRec0[0] = (fControl[1] * ((fControl[4] * fRec1[1]) + (fControl[5] * fRec0[1])));
    float fTemp0 = input;
    fRec2[0] = (fTemp0 - (fControl[15] * ((fControl[16] * fRec2[1]) + (fControl[17] * fRec2[2]))));
    fRec1[0] = (((fControl[14] * fRec2[2]) + ((fControl[5] * fRec1[1]) + (fControl[13] * fRec2[0]))) - (fControl[4] * fRec0[1]));
    float output = float((fControl[0] * fRec0[0]));
    fRec0[1] = fRec0[0];
    fRec2[2] = fRec2[1];
    fRec2[1] = fRec2[0];
    fRec1[1] = fRec1[0];
    return output;
}

} // namespace sfz
} // namespace fx
