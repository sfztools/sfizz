// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#define UNUSED(x) (void)(x)

#if __cplusplus > 201103L
#define CXX14_CONSTEXPR constexpr
#define CXX11_MOVE(x) x
#else
#define CXX14_CONSTEXPR
#define CXX11_MOVE(x) std::move(x)
#endif

