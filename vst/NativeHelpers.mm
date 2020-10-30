// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeHelpers.h"
#import <Foundation/Foundation.h>
#include <stdexcept>
#if !__has_feature(objc_arc)
#error This source file requires ARC
#endif

const fs::path& getUserDocumentsDirectory()
{
    static const fs::path directory = []() -> fs::path {
        NSFileManager* fm = [NSFileManager defaultManager];
        NSArray<NSURL*>* urls = [fm URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask];
        for (NSUInteger i = 0, n = [urls count]; i < n; ++i) {
            NSURL *url = [urls objectAtIndex:i];
            if ([url isFileURL])
                return fs::path([url path].UTF8String);
        }
        throw std::runtime_error("Cannot get the document directory.");
    }();
    return directory;
}
