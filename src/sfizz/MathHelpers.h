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
#include "Macros.h"
#include "SIMDConfig.h"
#include "absl/types/span.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <cfenv>
#if SFIZZ_HAVE_SSE
#include <xmmintrin.h>
#endif

template <class T>
constexpr T max(T op1, T op2)
{
    return op1 > op2 ? op1 : op2;
}

template <class T, class... Args>
constexpr T max(T op1, Args... rest)
{
    return max(op1, max(rest...));
}

template <class T>
constexpr T min(T op1, T op2)
{
    return op1 > op2 ? op2 : op1;
}

template <class T, class... Args>
constexpr T min(T op1, Args... rest)
{
    return min(op1, min(rest...));
}

/**
 * @brief Compute the square of the value
 *
 * @param op
 * @return T
 */
template <class T>
constexpr T power2(T in)
{
    return in * in;
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
    return 440.0f * std::pow(2.0f, (noteNumber - 69) * (1.0f / 12.0f));
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
template <class T>
constexpr T clamp(T v, T lo, T hi)
{
    return max(min(v, hi), lo);
}

template <int Increment = 1, class T>
inline CXX14_CONSTEXPR void incrementAll(T& only)
{
    only += Increment;
}

template <int Increment = 1, class T, class... Args>
inline CXX14_CONSTEXPR void incrementAll(T& first, Args&... rest)
{
    first += Increment;
    incrementAll<Increment>(rest...);
}

template <class ValueType>
constexpr ValueType linearInterpolation(const ValueType values[2], ValueType coeff)
{
    return values[0] * (static_cast<ValueType>(1.0) - coeff) + values[1] * coeff;
}

/**
 * @brief Compute the 3rd-order Hermite interpolation polynomial.
 *
 * @tparam R
 * @param x
 * @return R
 */
template <class R>
R hermite3(R x)
{
    x = std::abs(x);
    R x2 = x * x;
    R x3 = x2 * x;
    R y = 0;
    R q = R(5./2.) * x2; // a reoccurring term
    R p1 = R(1) - q + R(3./2.) * x3;
    R p2 = R(2) - R(4) * x + q - R(1./2.) * x3;
    y = (x < R(2)) ? p2 : y;
    y = (x < R(1)) ? p1 : y;
    return y;
}

#if SFIZZ_HAVE_SSE
/**
 * @brief Compute 4 parallel elements of the 3rd-order Hermite interpolation polynomial.
 *
 * @param x
 * @return __m128
 */
inline __m128 hermite3x4(__m128 x)
{
    x = _mm_andnot_ps(_mm_set1_ps(-0.0f), x);
    __m128 x2 = _mm_mul_ps(x, x);
    __m128 x3 = _mm_mul_ps(x2, x);
    __m128 y = _mm_set1_ps(0.0f);
    __m128 q = _mm_mul_ps(_mm_set1_ps(5./2.), x2);
    __m128 p1 = _mm_add_ps(_mm_sub_ps(_mm_set1_ps(1), q), _mm_mul_ps(_mm_set1_ps(3./2.), x3));
    __m128 p2 = _mm_sub_ps(_mm_add_ps(_mm_sub_ps(_mm_set1_ps(2), _mm_mul_ps(_mm_set1_ps(4), x)), q), _mm_mul_ps(_mm_set1_ps(1./2.), x3));
    __m128 m2 = _mm_cmple_ps(x, _mm_set1_ps(2));
    y = _mm_or_ps(_mm_and_ps(m2, p2), _mm_andnot_ps(m2, y));
    __m128 m1 = _mm_cmple_ps(x, _mm_set1_ps(1));
    y = _mm_or_ps(_mm_and_ps(m1, p1), _mm_andnot_ps(m1, y));
    return y;
}
#endif

template <class ValueType>
ValueType hermite3Interpolation(const ValueType values[4], ValueType coeff);

#if SFIZZ_HAVE_SSE
template <>
inline float hermite3Interpolation<float>(const float values[4], float coeff)
{
    __m128 x = _mm_sub_ps(_mm_setr_ps(-1, 0, 1, 2), _mm_set1_ps(coeff));
    __m128 h = hermite3x4(x);
    __m128 y = _mm_mul_ps(h, _mm_loadu_ps(values));
    // sum 4 to 1
    __m128 xmm0 = y;
    __m128 xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0xe5);
    __m128 xmm2 = _mm_movehl_ps(xmm0, xmm0);
    xmm1 = _mm_add_ss(xmm1, xmm0);
    xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0xe7);
    xmm2 = _mm_add_ss(xmm2, xmm1);
    xmm0 = _mm_add_ss(xmm0, xmm2);
    return _mm_cvtss_f32(xmm0);
}
#endif

template <class ValueType>
ValueType hermite3Interpolation(const ValueType values[4], ValueType coeff)
{
    ValueType y = 0;
    for (int i = 0; i < 4; ++i) {
        ValueType h = hermite3<ValueType>(i - 1 - coeff);
        y += h * values[i];
    }
    return y;
}

/**
 * @brief Compute the 3rd-order B-spline interpolation polynomial.
 *
 * @tparam R
 * @param x
 * @return R
 */
template <class R>
R bspline3(R x)
{
    x = std::abs(x);
    R x2 = x * x;
    R x3 = x2 * x;
    R y = 0;
    R p1 = R(2./3.) - x2 + R(1./2.) * x3;
    R p2 = R(4./3.) - R(2) * x + x2 - R(1./6.) * x3;
    y = (x < R(2)) ? p2 : y;
    y = (x < R(1)) ? p1 : y;
    return y;
}

#if SFIZZ_HAVE_SSE
/**
 * @brief Compute 4 parallel elements of the 3rd-order B-spline interpolation polynomial.
 *
 * @param x
 * @return __m128
 */
inline __m128 bspline3x4(__m128 x)
{
    x = _mm_andnot_ps(_mm_set1_ps(-0.0f), x);
    __m128 x2 = _mm_mul_ps(x, x);
    __m128 x3 = _mm_mul_ps(x2, x);
    __m128 y = _mm_set1_ps(0.0f);
    __m128 p1 = _mm_set1_ps(2./3.) - x2 + _mm_mul_ps(_mm_set1_ps(1./2.), x3);
    __m128 p2 = _mm_sub_ps(_mm_add_ps(_mm_sub_ps(_mm_set1_ps(4./3.), _mm_mul_ps(_mm_set1_ps(2), x)), x2), _mm_mul_ps(_mm_set1_ps(1./6.), x3));
    __m128 m2 = _mm_cmple_ps(x, _mm_set1_ps(2));
    y = _mm_or_ps(_mm_and_ps(m2, p2), _mm_andnot_ps(m2, y));
    __m128 m1 = _mm_cmple_ps(x, _mm_set1_ps(1));
    y = _mm_or_ps(_mm_and_ps(m1, p1), _mm_andnot_ps(m1, y));
    return y;
}
#endif

template <class ValueType>
ValueType bspline3Interpolation(const ValueType values[4], ValueType coeff);

#if SFIZZ_HAVE_SSE
template <>
inline float bspline3Interpolation<float>(const float values[4], float coeff)
{
    __m128 x = _mm_sub_ps(_mm_setr_ps(-1, 0, 1, 2), _mm_set1_ps(coeff));
    __m128 h = bspline3x4(x);
    __m128 y = _mm_mul_ps(h, _mm_loadu_ps(values));
    // sum 4 to 1
    __m128 xmm0 = y;
    __m128 xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0xe5);
    __m128 xmm2 = _mm_movehl_ps(xmm0, xmm0);
    xmm1 = _mm_add_ss(xmm1, xmm0);
    xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0xe7);
    xmm2 = _mm_add_ss(xmm2, xmm1);
    xmm0 = _mm_add_ss(xmm0, xmm2);
    return _mm_cvtss_f32(xmm0);
}
#endif

template <class ValueType>
ValueType bspline3Interpolation(const ValueType values[4], ValueType coeff)
{
    ValueType y = 0;
    for (int i = 0; i < 4; ++i) {
        ValueType h = bspline3<ValueType>(i - 1 - coeff);
        y += h * values[i];
    }
    return y;
}

template <class Type>
constexpr Type pi() { return static_cast<Type>(3.141592653589793238462643383279502884); };
template <class Type>
constexpr Type twoPi() { return pi<Type>() * 2; };
template <class Type>
constexpr Type piTwo() { return pi<Type>() / 2; };
template <class Type>
constexpr Type piFour() { return pi<Type>() / 4; };
template <class Type>
constexpr Type sqrtTwo() { return static_cast<Type>(1.414213562373095048801688724209698078569671875376948073176); };
template <class Type>
constexpr Type sqrtTwoInv() { return static_cast<Type>(0.707106781186547524400844362104849039284835937688474036588); };

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

template <>
struct FP_traits<double> {
    typedef double type;
    typedef uint64_t same_size_int;
    static_assert(sizeof(type) == sizeof(same_size_int),
        "Unexpected size of floating point type");
    static constexpr int e_bits = 11;
    static constexpr int m_bits = 52;
    static constexpr int e_offset = -1023;
};

template <>
struct FP_traits<float> {
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
    union {
        F real;
        typename T::same_size_int integer;
    } u;
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
    union {
        F real;
        typename T::same_size_int integer;
    } u;
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
    union {
        F real;
        typename T::same_size_int integer;
    } u;
    u.real = x;
    Fraction<uint64_t> f;
    f.den = uint64_t { 1 } << T::m_bits;
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
    union {
        F real;
        I integer;
    } u;
    u.integer = mant | (static_cast<I>(ex - T::e_offset) << T::m_bits) | (static_cast<I>(sgn) << (T::e_bits + T::m_bits));
    return u.real;
}

template <class F>
inline bool fp_naninf(F x)
{
    typedef FP_traits<F> T;
    typedef typename T::same_size_int I;
    union {
        F real;
        I integer;
    } u;
    u.real = x;
    const auto all_ones = ((1u << T::e_bits) - 1);
    const auto ex = (u.integer >> T::m_bits) & all_ones;
    return ex == all_ones;
}

template <class Type>
bool hasNanInf(absl::Span<Type> span)
{
    for (const auto& x : span)
        if (fp_naninf(x))
            return true;

    return false;
}

template <class Type>
bool isReasonableAudio(absl::Span<Type> span)
{
    for (const auto& x : span)
        if (x < -10.0f || x > 10.0f)
            return false;

    return true;
}

/**
 * @brief Finds the minimum size of 2 spans
 *
 * @tparam T
 * @tparam U
 * @param span1
 * @param span2
 * @return constexpr size_t
 */
template <class T, class U>
constexpr size_t minSpanSize(absl::Span<T>& span1, absl::Span<U>& span2)
{
    return min(span1.size(), span2.size());
}

/**
 * @brief Finds the minimum size of a list of spans.
 *
 * @tparam T
 * @tparam Others
 * @param first
 * @param others
 * @return constexpr size_t
 */
template <class T, class... Others>
constexpr size_t minSpanSize(absl::Span<T>& first, Others... others)
{
    return min(first.size(), minSpanSize(others...));
}

template <class T>
constexpr bool _checkSpanSizes(size_t size, absl::Span<T>& span1)
{
    return span1.size() == size;
}

template <class T, class... Others>
constexpr bool _checkSpanSizes(size_t size, absl::Span<T>& span1, Others... others)
{
    return span1.size() == size && _checkSpanSizes(size, others...);
}

/**
 * @brief Check that all spans of a compile time list have the same size
 *
 * @tparam T
 * @tparam Others
 * @param first
 * @param others
 * @return constexpr size_t
 */
template <class T, class... Others>
constexpr bool checkSpanSizes(const absl::Span<T>& span1, Others... others)
{
    return _checkSpanSizes(span1.size(), others...);
}

#define CHECK_SPAN_SIZES(...) CHECK(checkSpanSizes(__VA_ARGS__))


class ScopedRoundingMode {
public:
    ScopedRoundingMode() = delete;
    ScopedRoundingMode(int newRoundingMode)
        : savedFloatMode(std::fegetround())
    {
        std::fesetround(newRoundingMode);
    }
    ~ScopedRoundingMode()
    {
        std::fesetround(savedFloatMode);
    }

private:
    const int savedFloatMode;
};
