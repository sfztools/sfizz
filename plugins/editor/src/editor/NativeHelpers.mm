// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>

static bool openFileWithApplication(const char *fileName, NSString *application)
{
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSString* fileNameNs = [NSString stringWithUTF8String:fileName];
    return [workspace openFile:fileNameNs withApplication:application] == YES;
}

bool openFileInExternalEditor(const char *fileName)
{
    NSURL* applicationURL = (__bridge_transfer NSURL*)LSCopyDefaultApplicationURLForContentType(
        kUTTypePlainText, kLSRolesEditor, nil);
    if (!applicationURL || ![applicationURL isFileURL])
        return false;
    return openFileWithApplication(fileName, [applicationURL path]);
}

bool openDirectoryInExplorer(const char *fileName)
{
    return openFileWithApplication(fileName, @"Finder");
}

bool askQuestion(const char *text)
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:text]];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    NSInteger button = [alert runModal];
    return button == NSAlertFirstButtonReturn;
}
#endif
