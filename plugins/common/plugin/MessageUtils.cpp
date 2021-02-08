// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "MessageUtils.h"
#include <absl/strings/ascii.h>
#include <absl/strings/numbers.h>
#include <cstring>

namespace Messages {

bool matchOSC(const char* pattern, const char* path, unsigned* indices)
{
    unsigned nthIndex = 0;

    while (const char *endp = std::strchr(pattern, '&')) {
        size_t length = endp - pattern;
        if (std::strncmp(pattern, path, length))
            return false;
        pattern += length;
        path += length;

        length = 0;
        while (absl::ascii_isdigit(path[length]))
            ++length;

        if (!absl::SimpleAtoi(absl::string_view(path, length), &indices[nthIndex++]))
            return false;

        pattern += 1;
        path += length;
    }

    return !std::strcmp(path, pattern);
}

} // namespace Messages
