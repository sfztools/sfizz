// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace sfz {

enum InterpolatorModel : int {
    // a nearest interpolator
    kInterpolatorNearest,
    // a linear interpolator
    kInterpolatorLinear,
    // a Hermite 3rd order interpolator
    kInterpolatorHermite3,
    // a B-spline 3rd order interpolator
    kInterpolatorBspline3,
};

/**
 * @brief Interpolate from a vector of values
 *
 * @tparam M the interpolator model
 * @tparam R the sample type
 * @param values Pointer to a value in a larger vector of values.
 *               Depending on the interpolator the algorithm may
 *               read samples before and after. Usually you need
 *               to ensure that you have order - 1 samples available
 *               before and after the pointer, padding if necessary.
 * @param coeff the interpolation coefficient
 * @return R
 */
template <InterpolatorModel M, class R>
R interpolate(const R* values, R coeff);

} // namespace sfz

#include "Interpolators.hpp"
