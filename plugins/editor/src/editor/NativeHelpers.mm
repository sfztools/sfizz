// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>

bool openFileInExternalEditor(const char *fileNameUTF8)
{
    BOOL wasOpened = NO;

    NSURL* applicationURL = (__bridge_transfer NSURL*)LSCopyDefaultApplicationURLForContentType(
        kUTTypePlainText, kLSRolesEditor, nil);
    if (!applicationURL)
        return false;
    if ([applicationURL isFileURL]) {
        NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
        NSString* fileName = [NSString stringWithUTF8String:fileNameUTF8];
        wasOpened = [workspace openFile:fileName withApplication:[applicationURL path]];
    }

    return wasOpened == YES;
}
#endif
