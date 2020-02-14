// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @file MathHelpers.h
 * @author Paul Ferrand (paul@ferrand.cc)
 * @brief Contains math helper functions and math constants
 * @version 0.1
 * @date 2019-11-23
 */
#pragma once
#include "Config.h"
#include <algorithm>
#include <cmath>
#include <random>

template<class T>
constexpr T max(T op1, T op2)
{
    return std::max(op1, op2);
}

template<class T, class... Args>
constexpr T max(T op1, Args... rest)
{
    return std::max(op1, max(rest...));
}

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
	static std::minstd_rand randomGenerator { randomDevice() };
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
constexpr T clamp( T v, T lo, T hi )
{
    v = std::min(v, hi);
    v = std::max(v, lo);
    return v;
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
constexpr Type pi { static_cast<Type>(3.141592653589793238462643383279502884) };
template <class Type>
constexpr Type twoPi { static_cast<Type>(2) * pi<Type> };
template <class Type>
constexpr Type piTwo { pi<Type> / static_cast<Type>(2) };
template <class Type>
constexpr Type piFour { pi<Type> / static_cast<Type>(4) };
template <class Type>
constexpr Type sqrtTwo { static_cast<Type>(1.414213562373095048801688724209698078569671875376948073176) };
template <class Type>
constexpr Type sqrtTwoInv { static_cast<Type>(0.707106781186547524400844362104849039284835937688474036588) };
