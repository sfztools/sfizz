// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ScopedFTZ.h"
#if (HAVE_X86INTRIN_H)
#include <x86intrin.h>
#elif (HAVE_INTRIN_H)
#include <intrin.h>
#elif (HAVE_ARM_NEON_H)
#include "arm_neon.h"
#endif


ScopedFTZ::ScopedFTZ()
{
#if (HAVE_X86INTRIN_H || HAVE_INTRIN_H)
    unsigned mask = _MM_DENORMALS_ZERO_MASK | _MM_FLUSH_ZERO_MASK;
    registerState = _mm_getcsr();
    _mm_setcsr((registerState & (~mask)) | mask);
#elif HAVE_ARM_NEON_H
    intptr_t mask = (1 << 24);
    asm volatile("vmrs %0, fpscr" : "=r"(registerState));
    asm volatile("vmsr fpscr, %0" : : "ri"((registerState & (~mask)) | mask));
#endif
}

ScopedFTZ::~ScopedFTZ()
{
#if (HAVE_X86INTRIN_H || HAVE_INTRIN_H)
    _mm_setcsr(registerState);
#elif HAVE_ARM_NEON_H
    asm volatile("vmrs %0, fpscr" : : "ri"(registerState));
#endif
}
