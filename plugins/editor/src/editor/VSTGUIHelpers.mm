// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "VSTGUIHelpers.h"

#if defined(__APPLE__)
#include "vstgui/lib/platform/mac/macfactory.h"
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>

fs::path getResourceBasePath()
{
    CFBundleRef bundle = VSTGUI::getPlatformFactory().asMacFactory()->getBundle();
    if (!bundle)
        return fs::path();

    NSURL* url = (__bridge_transfer NSURL*)CFBundleCopyResourcesDirectoryURL(bundle);
    if (!url || ![url isFileURL])
        return fs::path();

    return fs::u8path([[url path] UTF8String]);
}
#endif
