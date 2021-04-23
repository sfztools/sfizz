// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
  Note: this file provides the traditional VST2 entry point `main` on Windows.
        some hosts only detect this entry point.

  It's required to compile this file in C mode.
  Under MinGW, having a signature for `main` which is different from the
  standard one is a warning in C, and an error in C++.
*/

#include "pluginterfaces/base/fplatform.h"
#include <stdint.h>

typedef struct AEffect AEffect;
typedef int32_t VstInt32;
typedef intptr_t VstIntPtr;
typedef intptr_t (__cdecl *audioMasterCallback) (AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

#if defined(_WIN32)
SMTG_EXPORT_SYMBOL AEffect* MAIN (audioMasterCallback audioMaster);

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wmain"
#endif

SMTG_EXPORT_SYMBOL AEffect* main (audioMasterCallback audioMaster)
{
    return MAIN(audioMaster);
}

#if defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

#endif
