// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeWindowHelpers.h"
#import <Cocoa/Cocoa.h>

namespace NativeWindows {

PuglRect getFrame(void* nativeWindowId)
{
    PuglRect frame;

    NSView* v = reinterpret_cast<NSView*>(nativeWindowId);
    NSRect r = [v frame];
    frame.x = r.origin.x;
    frame.y = r.origin.y;
    frame.width = r.size.width;
    frame.height = r.size.height;

    return frame;
}

} // namespace NativeWindows
