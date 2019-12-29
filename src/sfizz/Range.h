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
    // constexpr Range(std::initializer_list<Type> list)
    // {
    //     switch(list.size())
    //     {
    //     case 0:
    //         break;
    //     case 1:
    //         _start = *list.begin();
    //         _end = _start;
    //         break;
    //     default:
    //         _start = *list.begin();
    //         _end = *(list.begin() + 1);
    //     }
    // }
    constexpr Range(Type start, Type end) noexcept
        : _start(start)
        , _end(std::max(start, end))
    {
    }
    ~Range() = default;
    Type getStart() const noexcept { return _start; }
    Type getEnd() const noexcept { return _end; }
    /**
     * @brief Get the range as an std::pair of the endpoints
     *
     * @return std::pair<Type, Type>
     */
    std::pair<Type, Type> getPair() const noexcept { return std::make_pair<Type, Type>(_start, _end); }
    Range(const Range<Type>& range) = default;
    Range(Range<Type>&& range) = default;
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
