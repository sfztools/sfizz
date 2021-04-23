// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzSettings.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#if !__has_feature(objc_arc)
#error This source file requires ARC
#endif

static NSUserDefaults* getUserDefaults()
{
    return [[NSUserDefaults alloc] initWithSuiteName:@"tools.sfz.sfizz"];;
}

absl::optional<std::string> SfizzSettings::load(const char* key)
{
    NSUserDefaults* ud = getUserDefaults();
    NSString* value = [ud stringForKey:[NSString stringWithUTF8String:key]];
    if (!value)
        return {};
    return std::string(value.UTF8String);
}

bool SfizzSettings::store(const char* key, absl::string_view value)
{
    NSUserDefaults* ud = getUserDefaults();
    NSString* object =
        [[NSString alloc] initWithBytes:value.data()
         length:(NSUInteger)value.size() encoding:NSUTF8StringEncoding];
    [ud setObject:object forKey:[NSString stringWithUTF8String:key]];
    return true;
}
#endif
