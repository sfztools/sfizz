// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "VSTGUIHelpers.h"
#include "utility/vstgui_before.h"
#include "vstgui/lib/platform/platformfactory.h"
#if defined(_WIN32)
#   include "vstgui/lib/platform/win32/win32factory.h"
#elif defined(__APPLE__)
#else
#   include "vstgui/lib/platform/linux/linuxfactory.h"
#endif
#include "utility/vstgui_after.h"

using namespace VSTGUI;

#if defined(_WIN32)
fs::path getResourceBasePath()
{
    Optional<UTF8String> optionalPath = getPlatformFactory().asWin32Factory()->getResourceBasePath();
    if (!optionalPath)
        return fs::path();
    return fs::u8path(optionalPath->getString());
}
#elif defined(__APPLE__)
    // implemented in VSTGUIHelpers.mm
#else
fs::path getResourceBasePath()
{
    return fs::u8path(getPlatformFactory().asLinuxFactory()->getResourcePath());
}
#endif
