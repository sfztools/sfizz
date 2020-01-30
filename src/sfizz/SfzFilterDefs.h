// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

namespace sfz {

enum {
    /**
       Minimum interval in frames between recomputations of coefficients of the
       modulated filter. The lower, the more CPU resources are consumed.
    */
    kFilterControlInterval = 16,
};

} // namespace sfz
