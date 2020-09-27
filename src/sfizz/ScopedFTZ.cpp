// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ScopedFTZ.h"
#include "SIMDConfig.h"
#if SFIZZ_HAVE_SSE
#include <pmmintrin.h> // x86 CSR is SSE, but FTZ bits SSE3 and up
#elif SFIZZ_HAVE_NEON
#include "arm_neon.h"
#endif


ScopedFTZ::ScopedFTZ()
{
#if SFIZZ_HAVE_SSE
    unsigned mask = _MM_DENORMALS_ZERO_MASK | _MM_FLUSH_ZERO_MASK;
    registerState = _mm_getcsr();
    _mm_setcsr((registerState & (~mask)) | mask);
#elif SFIZZ_HAVE_NEON && SFIZZ_CPU_FAMILY_ARM
    intptr_t mask = (1 << 24);
    asm volatile("vmrs %0, fpscr" : "=r"(registerState));
    asm volatile("vmsr fpscr, %0" : : "r"((registerState & (~mask)) | mask));
#elif SFIZZ_HAVE_NEON && SFIZZ_CPU_FAMILY_AARCH64
    intptr_t mask = (1 << 24);
    asm volatile("mrs %0, fpcr" : "=r"(registerState));
    asm volatile("msr fpcr, %0" : : "r"((registerState & (~mask)) | mask));
#endif
}

ScopedFTZ::~ScopedFTZ()
{
#if SFIZZ_HAVE_SSE
    _mm_setcsr(registerState);
#elif SFIZZ_HAVE_NEON && SFIZZ_CPU_FAMILY_ARM
    asm volatile("vmrs %0, fpscr" : : "r"(registerState));
#elif SFIZZ_HAVE_NEON && SFIZZ_CPU_FAMILY_AARCH64
    asm volatile("mrs %0, fpcr" : : "r"(registerState));
#endif
}
