// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * @file StringViewHelpers.h
 * @author Paul Ferrand (paul@ferrand.cc)
 * @brief Contains some helper functions for string views
 * @version 0.1
 * @date 2019-11-23
 *
 * @copyright Copyright (c) 2019
 *
 */

#pragma once
#include "absl/strings/string_view.h"

/**
 * @brief Removes the whitespace on a string_view in place
 *
 * @param s
 */
inline void trimInPlace(absl::string_view& s)
{
    const auto leftPosition = s.find_first_not_of(" \r\t\n\f\v");
    if (leftPosition != s.npos) {
        s.remove_prefix(leftPosition);
        const auto rightPosition = s.find_last_not_of(" \r\t\n\f\v");
        s.remove_suffix(s.size() - rightPosition - 1);
    } else {
        s.remove_suffix(s.size());
    }
}

/**
 * @brief Removes the whitespace on a string_view and return a new string_view
 *
 * @param s
 * @return absl::string_view
 */
inline absl::string_view trim(absl::string_view s)
{
    trimInPlace(s);
    return s;
}

constexpr uint64_t Fnv1aBasis = 0x811C9DC5;
constexpr uint64_t Fnv1aPrime = 0x01000193;

/**
 * @brief Compile-time hashing function to be used mostly with switch/case statements.
 *
 * See e.g. the Region.cpp file
 *
 * @param s the input string to be hashed
 * @param h the hashing seed to use
 * @return uint64_t
 */
constexpr uint64_t hash(absl::string_view s, uint64_t h = Fnv1aBasis)
{
    if (s.length() > 0)
        return hash( { s.data() + 1, s.length() - 1 }, (h ^ s.front()) * Fnv1aPrime );

    return h;
}
