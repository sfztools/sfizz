// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "MathHelpers.h"
#include <initializer_list>
#include <type_traits>

namespace sfz
{
/**
 * @brief This class holds a range with functions to clamp and test if a value is in the range
 *
 * @tparam Type
 */
template <class Type>
class Range {
    static_assert(std::is_arithmetic<Type>::value, "The Type should be arithmetic");

public:
    constexpr Range() = default;
    constexpr Range(Type start, Type end) noexcept
        : _start(start)
        , _end(max(start, end))
    {

    }
    Type getStart() const noexcept { return _start; }
    Type getEnd() const noexcept { return _end; }
    /**
     * @brief Get the range as an std::pair of the endpoints
     *
     * @return std::pair<Type, Type>
     */
    std::pair<Type, Type> getPair() const noexcept { return std::make_pair<Type, Type>(_start, _end); }
    constexpr Type length() const { return _end - _start; }
    void setStart(Type start) noexcept
    {
        _start = start;
        if (start > _end)
            _end = start;
    }

    void setEnd(Type end) noexcept
    {
        _end = end;
        if (end < _start)
            _start = end;
    }
    /**
     * @brief Clamp a value within the range including the endpoints
     *
     * @param value
     * @return Type
     */
    constexpr Type clamp(Type value) const noexcept { return ::clamp(value, _start, _end); }
    /**
     * @brief Checks if a value is in the range, including the endpoints
     *
     * @param value
     * @return true
     * @return false
     */
    bool containsWithEnd(Type value) const noexcept { return (value >= _start && value <= _end); }
    /**
     * @brief Checks if a value is in the range, excluding the end of the range
     *
     * @param value
     * @return true
     * @return false
     */
    bool contains(Type value) const noexcept { return (value >= _start && value < _end); }
    /**
     * @brief Shrink the region if it is smaller than the provided start and end points
     *
     * @param start
     * @param end
     */
    void shrinkIfSmaller(Type start, Type end)
    {
        if (start > end)
            std::swap(start, end);

        if (start > _start)
            _start = start;

        if (end < _end)
            _end = end;
    }

    void expandTo(Type value)
    {
        if (containsWithEnd(value))
            return;

        if (value > _end)
            _end = value;
        else
            _start = value;
    }

private:
    Type _start { static_cast<Type>(0.0) };
    Type _end { static_cast<Type>(0.0) };
};
}

template <class Type>
bool operator==(const sfz::Range<Type>& lhs, const sfz::Range<Type>& rhs)
{
    return (lhs.getStart() == rhs.getStart()) && (lhs.getEnd() == rhs.getEnd());
}

template <class Type>
bool operator!=(const sfz::Range<Type>& lhs, const sfz::Range<Type>& rhs)
{
    return (lhs.getStart() != rhs.getStart()) || (lhs.getEnd() != rhs.getEnd());
}


template <class Type>
bool operator==(const sfz::Range<Type>& lhs, const std::pair<Type, Type>& rhs)
{
    return (lhs.getStart() == rhs.first) && (lhs.getEnd() == rhs.second);
}

template <class Type>
bool operator==(const std::pair<Type, Type>& lhs, const sfz::Range<Type>& rhs)
{
    return rhs == lhs;
}
