#pragma once

#ifndef NDEBUG
#include <iostream>
// These trap into the signal library rather than your own sourcecode
// #include <signal.h>
// #define ASSERTFALSE { ::kill(0, SIGTRAP); }
// #define ASSERTFALSE { raise(SIGTRAP); }
#if (__linux__ || __unix__)

// Break in source code
#if (__x86_64__ || __i386__)
#define ASSERTFALSE      \
    {                    \
        __asm__("int3"); \
    }
#elif (__arm__ || __aarch64__)
#define ASSERTFALSE       \
    {                     \
        __builtin_trap(); \
    }
#endif

#elif (_WIN32 || _WIN64)
#pragma intrinsic(__debugbreak)
#define ASSERTFALSE     \
    {                   \
        __debugbreak(); \
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