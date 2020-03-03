// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Curve.h"
#include "MathHelpers.h"

namespace sfz
{

inline float Curve::evalCC7(int value7) const
{
    return _points[clamp(value7, 0, 127)];
}

inline float Curve::evalCC7(float value7) const
{
    value7 = clamp(value7, 0.0f, 127.0f);
    int i1 = static_cast<int>(value7);
    int i2 = std::min(127, i1 + 1);
    float mu = value7 - i1;
    return _points[i1] + mu * (_points[i2] - _points[i1]);
}

inline float Curve::evalNormalized(float value) const
{
    return evalCC7(127 * value);
}

} // namespace sfz
