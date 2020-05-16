// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace sfz {

enum InterpolatorModel : int {
    // a linear interpolator
    kInterpolatorLinear,
    // a Hermite 3rd order interpolator
    kInterpolatorHermite3,
    // a B-spline 3rd order interpolator
    kInterpolatorBspline3,
};

template <InterpolatorModel M, class R>
R interpolate(const R* values, R coeff);

} // namespace sfz

#include "Interpolators.hpp"
