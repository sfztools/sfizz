// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Synth.h"
#include "StringViewHelpers.h"
#include <absl/strings/ascii.h>
#include <cstring>

namespace sfz {
static constexpr unsigned maxIndices = 8;

static bool extractMessage(const char* pattern, const char* path, unsigned* indices);
static uint64_t hashMessagePath(const char* path, const char* sig);

void sfz::Synth::dispatchMessage(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    unsigned indices[maxIndices];

    switch (hashMessagePath(path, sig)) {
        #define MATCH(p, s) case hash(p "," s): \
            if (extractMessage(p, path, indices) && !strcmp(sig, s))

        MATCH("/hello", "") {
            client.receive(delay, "/hello", "", nullptr);
            break;
        }

        // TODO...
    }
}

static bool extractMessage(const char* pattern, const char* path, unsigned* indices)
{
    unsigned nthIndex = 0;

    while (const char *endp = strchr(pattern, '&')) {
        if (nthIndex == maxIndices)
            return false;

        size_t length = endp - pattern;
        if (strncmp(pattern, path, length))
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

    return !strcmp(path, pattern);
}

static uint64_t hashMessagePath(const char* path, const char* sig)
{
    uint64_t h = Fnv1aBasis;
    while (unsigned char c = *path++) {
        if (!absl::ascii_isdigit(c))
            h = hashByte(c, h);
        else {
            h = hashByte('&', h);
            while (absl::ascii_isdigit(*path))
                ++path;
        }
    }
    h = hashByte(',', h);
    while (unsigned char c = *sig++)
        h = hashByte(c, h);
    return h;
}

} // namespace sfz
