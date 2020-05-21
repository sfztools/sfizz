// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeWindowHelpers.h"
#include <X11/Xlib.h>

namespace NativeWindows {

PuglRect getFrame(void* nativeWindowId)
{
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy)
        return {};

    PuglRect frame;

    Window w = reinterpret_cast<Window>(nativeWindowId);

    Window root;
    int x;
    int y;
    unsigned width;
    unsigned height;
    unsigned borderWidth;
    unsigned borderHeight;
    if (XGetGeometry(dpy, w, &root, &x, &y, &width, &height, &borderWidth, &borderHeight) == 0)
        return {};

    frame.x = x;
    frame.y = y;
    frame.width = width;
    frame.height = height;

    XCloseDisplay(dpy);

    return frame;
}

} // namespace NativeWindows
