// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeWindowHelpers.h"
#include <windows.h>

namespace NativeWindows {

PuglRect getFrame(void* nativeWindowId)
{
    PuglRect frame;

    HWND w = reinterpret_cast<HWND>(nativeWindowId);
    RECT r;
    GetClientRect(w, &r);
    frame.x = r.left;
    frame.y = r.top;
    frame.width = r.right - r.left;
    frame.height = r.bottom - r.top;

    return frame;
}

} // namespace NativeWindows
