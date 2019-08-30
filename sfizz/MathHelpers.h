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
#include <algorithm>
#include <cmath>
#include <random>

template <class T>
inline constexpr T min(T op1, T op2) { return std::min(op1, op2); }
template <class T>
inline constexpr T min(T op1, T op2, T op3) { return std::min(op1, std::min(op2, op3)); }
template <class T>
inline constexpr T min(T op1, T op2, T op3, T op4) { return std::min(op1, std::min(op2, std::min(op3, op4))); }


template <class Type>
inline constexpr Type db2pow(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.1));
}

template <class Type>
inline constexpr Type pow2db(Type in)
{
    return static_cast<Type>(10.0) * std::log10(in);
}

template <class Type>
inline constexpr Type db2mag(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.05));
}

template <class Type>
inline constexpr Type mag2db(Type in)
{
    return static_cast<Type>(20.0) * std::log10(in);
}

namespace Random {
static inline std::random_device randomDevice;
static inline std::mt19937 randomGenerator { randomDevice() };
} // namespace Random

inline float midiNoteFrequency(const int noteNumber)
{
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

template <class Type>
constexpr Type pi { 3.141592653589793238462643383279502884 };
template <class Type>
constexpr Type twoPi { 2 * pi<Type> };
template <class Type>
constexpr Type piTwo { pi<Type> / 2 };