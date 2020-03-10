// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @file MathHelpers.h
 * @brief Contains math helper functions and math constants
 */
#pragma once
#include "Config.h"
#include <algorithm>
#include <cmath>
#include <random>

template<class T>
constexpr T max(T op1, T op2)
{
    return op1 > op2 ? op1 : op2;
}

template<class T, class... Args>
constexpr T max(T op1, Args... rest)
{
    return max(op1, max(rest...));
}

template<class T>
constexpr T min(T op1, T op2)
{
    return op1 > op2 ? op2 : op1;
}

template<class T, class... Args>
constexpr T min(T op1, Args... rest)
{
    return min(op1, min(rest...));
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
    v = min(v, hi);
    v = max(v, lo);
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

constexpr double dPi { 3.141592653589793238462643383279502884};
constexpr double dTwoPi { dPi * 2 };
constexpr double dPiTwo { dPi / 2 };
constexpr double dPiFour { dPi / 4 };
constexpr double dSqrtTwo { 1.414213562373095048801688724209698078569671875376948073176 };
constexpr double dSqrtTwoInv { 0.707106781186547524400844362104849039284835937688474036588 };

constexpr float fPi { 3.141592653589793238462643383279502884};
constexpr float fTwoPi { fPi * 2 };
constexpr float fPiTwo { fPi / 2 };
constexpr float fPiFour { fPi / 4 };
constexpr float fSqrtTwo { 1.414213562373095048801688724209698078569671875376948073176 };
constexpr float fSqrtTwoInv { 0.707106781186547524400844362104849039284835937688474036588 };

/**
   @brief A fraction which is parameterized by integer type
 */
template <class I>
struct Fraction {
    typedef I value_type;

    operator double() const noexcept;
    operator float() const noexcept;

    I num;
    I den;
};

template <class I>
inline Fraction<I>::operator double() const noexcept
{
    return static_cast<double>(num) / static_cast<double>(den);
}

template <class I>
inline Fraction<I>::operator float() const noexcept
{
    return static_cast<float>(num) / static_cast<float>(den);
}

/**
   @brief Characteristics of IEEE754 floating point representations
 */
template <class F>
struct FP_traits;

template <> struct FP_traits<double>
{
    typedef double type;
    typedef uint64_t same_size_int;
    static_assert(sizeof(type) == sizeof(same_size_int),
                  "Unexpected size of floating point type");
    static constexpr int e_bits = 11;
    static constexpr int m_bits = 52;
    static constexpr int e_offset = -1023;
};

template <> struct FP_traits<float>
{
    typedef float type;
    typedef uint32_t same_size_int;
    static_assert(sizeof(type) == sizeof(same_size_int),
                  "Unexpected size of floating point type");
    static constexpr int e_bits = 8;
    static constexpr int m_bits = 23;
    static constexpr int e_offset = -127;
};

/**
   @brief Get the sign part of a IEEE754 floating point number.

   The number is reconstructed as `(-1^sign)*(1+mantissa)*(2^exponent)`.
   See also `fp_exponent` and `fp_mantissa`.
 */
template <class F>
inline bool fp_sign(F x)
{
    typedef FP_traits<F> T;
    union { F real; typename T::same_size_int integer; } u;
    u.real = x;
    return ((u.integer >> (T::e_bits + T::m_bits)) & 1) != 0;
}

/**
   @brief Get the exponent part of a IEEE754 floating point number.

   The number is reconstructed as `(-1^sign)*(1+mantissa)*(2^exponent)`.
   See also `fp_sign` and `fp_mantissa`.

   It is a faster way of computing `floor(log2(abs(x)))`.
 */
template <class F>
inline int fp_exponent(F x)
{
    typedef FP_traits<F> T;
    union { F real; typename T::same_size_int integer; } u;
    u.real = x;
    int ex = (u.integer >> T::m_bits) & ((1u << T::e_bits) - 1);
    return ex + T::e_offset;
}

/**
   @brief Get the mantissa part of a IEEE754 floating point number.
   The number is reconstructed as `(-1^sign)*(1+mantissa)*(2^exponent)`.
   See also `fp_sign` and `fp_exponent`.
 */
template <class F>
inline Fraction<uint64_t> fp_mantissa(F x)
{
    typedef FP_traits<F> T;
    union { F real; typename T::same_size_int integer; } u;
    u.real = x;
    Fraction<uint64_t> f;
    f.den = uint64_t{1} << T::m_bits;
    f.num = u.integer & (f.den - 1);
    return f;
}

/**
   @brief Reconstruct a IEEE754 floating point number from its parts.

   The parts must be in their range of validity.
 */
template <class F>
inline F fp_from_parts(bool sgn, int ex, uint64_t mant)
{
    typedef FP_traits<F> T;
    typedef typename T::same_size_int I;
    union { F real; I integer; } u;
    u.integer = mant |
        (static_cast<I>(ex - T::e_offset) << T::m_bits) |
        (static_cast<I>(sgn) << (T::e_bits + T::m_bits));
    return u.real;
}
