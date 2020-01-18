// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
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
