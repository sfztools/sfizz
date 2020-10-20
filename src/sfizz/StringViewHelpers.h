// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @file StringViewHelpers.h
 * @brief Contains some helper functions for string views
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
    return (s.length() == 0) ? h : hash( { s.data() + 1, s.length() - 1 }, (h ^ static_cast<unsigned char>(s.front())) * Fnv1aPrime );
}

/**
 * @brief Same function as `hash()` but ignores ampersands (&)
 *
 * See e.g. the Region.cpp file
 *
 * @param s the input string to be hashed
 * @param h the hashing seed to use
 * @return uint64_t
 */
constexpr uint64_t hashNoAmpersand(absl::string_view s, uint64_t h = Fnv1aBasis)
{
    return (s.length() == 0) ? h : (
        (s.front() == '&')
            ? hashNoAmpersand( { s.data() + 1, s.length() - 1 }, h )
            : hashNoAmpersand( { s.data() + 1, s.length() - 1 }, (h ^ static_cast<unsigned char>(s.front())) * Fnv1aPrime )
    );
}

/**
 * @brief Run-time hashing function for numbers, useful for example to
 *        create hash functions for keys which depend on numeric values.
 */
template <class Int>
uint64_t hashNumber(Int i, uint64_t h = Fnv1aBasis)
{
    static_assert(std::is_arithmetic<Int>::value,
                  "The hashed object must be of arithmetic type");
    return hash(absl::string_view(reinterpret_cast<const char*>(&i), sizeof(i)), h);
}
