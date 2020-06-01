// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

template<class T>
inline void readInterleavedScalar(const T* input, T* outputLeft, T* outputRight, unsigned inputSize)
{
    const auto sentinel = input + inputSize - 1;
    while (input < sentinel) {
        *outputLeft++ = *input++;
        *outputRight++ = *input++;
    }
}

template<class T>
inline void writeInterleavedScalar(const T* inputLeft, const T* inputRight, T* output, unsigned outputSize)
{
    const auto sentinel = output + outputSize - 1;
    while (output < sentinel) {
        *output++ = *inputLeft++;
        *output++ = *inputRight++;
    }
}

template<class T>
inline void applyGainScalar(T gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = gain * (*input++);
}

template<class T>
inline void applyGainScalar(const T* gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = (*gain++) * (*input++);
}

template <class T>
inline void divideScalar(const T* input, const T* divisor, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ = (*input++) / (*divisor++);
}

template <class T>
inline void multiplyAddScalar(const T* gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += (*gain++) * (*input++);
}

template <class T>
inline void multiplyAddScalar(T gain, const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += gain * (*input++);
}

template <class T>
T linearRampScalar(T* output, T start, T step, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel) {
        *output++ = start;
        start += step;
    }
    return start;
}

template <class T>
T multiplicativeRampScalar(T* output, T start, T step, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel) {
        *output++ = start;
        start *= step;
    }
    return start;
}

template <class T>
inline void addScalar(const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += *input++;
}

template <class T>
inline void addScalar(T value, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ += value;
}

template <class T>
inline void subtractScalar(const T* input, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ -= *input++;
}

template <class T>
inline void subtractScalar(T value, T* output, unsigned size) noexcept
{
    const auto sentinel = output + size;
    while (output < sentinel)
        *output++ -= value;
}

template <class T>
T meanScalar(const T* vector, unsigned size) noexcept
{
    T result{ 0.0 };
    if (size == 0)
        return result;

    const auto sentinel = vector + size;
    while (vector < sentinel)
        result += *vector++;

    return result / static_cast<T>(size);
}

template <class T>
T meanSquaredScalar(const T* vector, unsigned size) noexcept
{
    T result{ 0.0 };
    if (size == 0)
        return result;

    const auto sentinel = vector + size;
    while (vector < sentinel) {
        result += (*vector) * (*vector);
        vector++;
    }

    return result / static_cast<T>(size);
}

template <class T>
void cumsumScalar(const T* input, T* output, unsigned size) noexcept
{
    if (size == 0)
        return;

    const auto sentinel = output + size;

    *output++ = *input++;
    while (output < sentinel) {
        *output = *(output - 1) + *input;
        incrementAll(input, output);
    }
}

template <class T>
void diffScalar(const T* input, T* output, unsigned size) noexcept
{
    if (size == 0)
        return;

    const auto sentinel = output + size;

    *output++ = *input++;
    while (output < sentinel) {
        *output = *input - *(input - 1);
        incrementAll(input, output);
    }
}
