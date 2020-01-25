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
    {                                                                            \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __asm__("int3");                                                         \
    }
#elif (__arm__ || __aarch64__)
#define ASSERTFALSE                                                              \
    {                                                                            \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __builtin_trap();                                                        \
    }
#endif

#elif (_WIN32 || _WIN64)
#pragma intrinsic(__debugbreak)
#define ASSERTFALSE                                                              \
    {                                                                            \
        std::cerr << "Assert failed at " << __FILE__ << ":" << __LINE__ << '\n'; \
        __debugbreak();                                                          \
    }
#else
#define ASSERTFALSE
#endif

// Assert stuff
#define ASSERT(expression) \
    if (!(expression))     \
    ASSERTFALSE

// Debug message
#define DBG(ostream) std::cerr << ostream << '\n'

#else // NDEBUG

#define ASSERTFALSE
#define ASSERT(expression)
#define DBG(ostream)

#endif
