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
 * @file MathHelpers.h
 * @author Paul Ferrand (paul@ferrand.cc)
 * @brief Contains math helper functions and math constants
 * @version 0.1
 * @date 2019-11-23
 */
#pragma once
#include <algorithm>
#include <cmath>
#include <cassert>
#include <random>

template<class T>
constexpr T min(T op1, T op2)
{
    return std::min(op1, op2);
}

template<class T, class... Args>
constexpr T min(T op1, Args... rest)
{
    return std::min(op1, min(rest...));
}

/**
 * @brief Converts db values into power (applies 10**(in/10))
 *
 * @tparam Type
 * @param in
 * @return Type
 */
template <class Type>
constexpr Type db2pow(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.1));
}

/**
 * @brief Converts power values into dB (applies 10log10(in))
 *
 * @tparam Type
 * @param in
 * @return Type
 */
template <class Type>
constexpr Type pow2db(Type in)
{
    return static_cast<Type>(10.0) * std::log10(in);
}

/**
 * @brief Converts dB values to magnitude (applies 10**(in/20))
 *
 * @tparam Type
 * @param in
 * @return constexpr Type
 */
template <class Type>
constexpr Type db2mag(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.05));
}

/**
 * @brief Converts magnitude values into dB (applies 20log10(in))
 *
 * @tparam Type
 * @param in
 * @return Type
 */
template <class Type>
constexpr Type mag2db(Type in)
{
    return static_cast<Type>(20.0) * std::log10(in);
}

/**
 * @brief Global random singletons
 *
 * TODO: could be moved into a singleton class holder
 *
 */
namespace Random {
	static std::random_device randomDevice;
	static std::mt19937 randomGenerator { randomDevice() };
} // namespace Random

/**
 * @brief Converts a midi note to a frequency value
 *
 * @param noteNumber
 * @return float
 */
inline float midiNoteFrequency(const int noteNumber)
{
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

/**
 * @brief Clamps a value between bounds, including the bounds!
 *
 * @tparam T
 * @param v
 * @param lo
 * @param hi
 * @return T
 */
template<class T>
constexpr T clamp( const T& v, const T& lo, const T& hi )
{
	assert( !(hi < lo) );
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

template<int Increment = 1, class T>
constexpr void incrementAll(T& only)
{
    only += Increment;
}

template<int Increment = 1, class T, class... Args>
constexpr void incrementAll(T& first, Args&... rest)
{
    first += Increment;
    incrementAll<Increment>(rest...);
}

template<class ValueType>
constexpr ValueType linearInterpolation(ValueType left, ValueType right, ValueType leftCoeff, ValueType rightCoeff)
{
    return left * leftCoeff + right * rightCoeff;
}

template <class Type>
constexpr Type pi { 3.141592653589793238462643383279502884 };
template <class Type>
constexpr Type twoPi { 2 * pi<Type> };
template <class Type>
constexpr Type piTwo { pi<Type> / 2 };
template <class Type>
constexpr Type piFour { pi<Type> / 4 };
template <class Type>
constexpr Type sqrtTwo { 1.414213562373095048801688724209698078569671875376948073176 };
template <class Type>
constexpr Type sqrtTwoInv { 0.707106781186547524400844362104849039284835937688474036588 };
