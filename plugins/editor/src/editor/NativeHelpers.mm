// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <cstring>

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

bool openURLWithExternalProgram(const char *url)
{
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* urlNs = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    return [workspace openURL:urlNs] == YES;
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

std::string getOperatingSystemName()
{
#if TARGET_OS_IOS
    NSString *osName = @"iOS";
#elif TARGET_OS_MAC
    NSString *osName = @"macOS";
#endif
    NSString *osVersion = [[NSProcessInfo processInfo] operatingSystemVersionString];
    return [[NSString stringWithFormat:@"%@ %@", osName, osVersion] UTF8String];
}

std::string getProcessorName()
{
    char nameBuf[256];
    size_t size = sizeof(nameBuf);
    const char* fallbackName = "Unknown";

    if (sysctlbyname("machdep.cpu.brand_string", nameBuf, &size, nullptr, 0) == -1)
        return fallbackName;

    size = strnlen(nameBuf, sizeof(nameBuf));
    if (size == 0)
        return fallbackName;

    return std::string(nameBuf, size);
}

std::string getCurrentProcessName()
{
    kinfo_proc proc {};
    size_t size = sizeof(kinfo_proc);
    int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, int(getpid()) };
    if (sysctl(name, 4, &proc, &size, nullptr, 0) == -1)
        return {};
    return proc.kp_proc.p_comm;
}
#endif
