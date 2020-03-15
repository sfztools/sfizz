// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#ifndef NDEBUG
#include <iostream>

#if (__linux__ || __unix__)

// Break in source code
#if (__x86_64__ || __i386__)
#define ASSERTFALSE                                                              \
    do {                                                                         \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __asm__("int3");                                                         \
    } while (0)
#elif (__arm__ || __aarch64__)
#define ASSERTFALSE                                                              \
    do {                                                                         \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __builtin_trap();                                                        \
    } while (0)
#endif

#elif (_WIN32 || _WIN64)
#ifdef _MSC_VER
#pragma intrinsic(__debugbreak)
#endif
#define ASSERTFALSE                                                              \
    do {                                                                         \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __debugbreak();                                                          \
    } while (0)
#else
#define ASSERTFALSE do {} while (0)
#endif

// Assert stuff
#define ASSERT(expression) \
    do {                   \
        if (!(expression)) \
            ASSERTFALSE;    \
    } \
    while (0)

// Debug message
#define DBG(ostream) do { std::cerr << ostream << '\n'; } while (0)

#else // NDEBUG

#define ASSERTFALSE do {} while (0)
#define ASSERT(expression) do {} while (0)
#define DBG(ostream) do {} while (0)

#endif
