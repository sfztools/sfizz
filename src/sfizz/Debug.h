// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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