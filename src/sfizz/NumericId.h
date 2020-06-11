// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

/**
 * @brief Numeric identifier
 *
 * It is a generic numeric identifier. The template wrapper serves to enforce a
 * stronger compile-time check, such that one kind of identifier can't be
 * mistaken for another kind, or for an unrelated integer such as an index.
 */
template <class T>
struct NumericId {
    constexpr NumericId() = default;

    explicit constexpr NumericId(int number)
        : number(number)
    {
    }

    constexpr bool valid() const noexcept
    {
        return number != -1;
    }

    const int number = -1;
};
