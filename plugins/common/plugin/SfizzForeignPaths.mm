// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzForeignPaths.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#if !__has_feature(objc_arc)
#error This source file requires ARC
#endif

namespace SfizzPaths {

fs::path getAriaPathSetting(const char* name)
{
    NSUserDefaults* ud = [[NSUserDefaults alloc] initWithSuiteName:@"com.plogue.aria"];
    NSString* value = [ud stringForKey:[NSString stringWithUTF8String:name]];
    if (!value)
        return {};
    return fs::path(value.UTF8String);
}

} // namespace SfizzPaths
#endif
