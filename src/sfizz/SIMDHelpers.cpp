// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include "SIMDConfig.h"
#include "Debug.h"
#include "simd/HelpersSSE.h"
#include "simd/HelpersAVX.h"
#include "cpuid/cpuinfo.hpp"
#include <array>
#include <mutex>

namespace sfz {

template <class T>
struct SIMDDispatch {
    constexpr SIMDDispatch() = default;

    void resetStatus();
    bool getStatus(SIMDOps op) const;
    void setStatus(SIMDOps op, bool enable);

    void (*writeInterleaved)(const T* inputLeft, const T* inputRight, T* output, unsigned outputSize) noexcept = &writeInterleavedScalar<float>;
    void (*readInterleaved)(const T* input, T* outputLeft, T* outputRight, unsigned inputSize) noexcept = &readInterleavedScalar<float>;
    void (*applyGain)(const T* gain, const T* input, T* output, unsigned size) noexcept = &applyGainScalar<float>;
    void (*applyGain1)(T gain, const T* input, T* output, unsigned size) noexcept = &applyGainScalar<float>;
    void (*divide)(const T* input, const T* divisor, T* output, unsigned size) noexcept = &divideScalar<float>;
    void (*multiplyAdd)(const T* gain, const T* input, T* output, unsigned size) noexcept = &multiplyAddScalar<float>;
    void (*multiplyAdd1)(T gain, const T* input, T* output, unsigned size) noexcept = &multiplyAddScalar<float>;
    T (*linearRamp)(T* output, T start, T step, unsigned size) noexcept = &linearRampScalar<float>;
    T (*multiplicativeRamp)(T* output, T start, T step, unsigned size) noexcept = &multiplicativeRampScalar<float>;
    void (*add)(const T* input, T* output, unsigned size) noexcept = &addScalar<float>;
    void (*add1)(T value, T* output, unsigned size) noexcept = &addScalar<float>;
    void (*subtract)(const T* input, T* output, unsigned size) noexcept = &subtractScalar<float>;
    void (*subtract1)(T value, T* output, unsigned size) noexcept = &subtractScalar<float>;
    void (*copy)(const T* input, T* output, unsigned size) noexcept = &copyScalar<float>;
    void (*cumsum)(const T* input, T* output, unsigned size) noexcept = &cumsumScalar<float>;
    void (*diff)(const T* input, T* output, unsigned size) noexcept = &diffScalar<float>;
    T (*mean)(const T* vector, unsigned size) noexcept = &meanScalar<float>;
    T (*meanSquared)(const T* vector, unsigned size) noexcept = &meanSquaredScalar<float>;

private:
    std::array<bool, static_cast<unsigned>(SIMDOps::_sentinel)> simdStatus;
};

///

static SIMDDispatch<float> simdDispatch;

void resetSIMDOpStatus()
{
    simdDispatch.resetStatus();
}

void setSIMDOpStatus(SIMDOps op, bool status)
{
    simdDispatch.setStatus(op, status);
}

bool getSIMDOpStatus(SIMDOps op)
{
    return simdDispatch.getStatus(op);
}

///

void readInterleaved(const float* input, float* outputLeft, float* outputRight, unsigned inputSize) noexcept
{
    return simdDispatch.readInterleaved(input, outputLeft, outputRight, inputSize);
}

void writeInterleaved(const float* inputLeft, const float* inputRight, float* output, unsigned outputSize) noexcept
{
    return simdDispatch.writeInterleaved(inputLeft, inputRight, output, outputSize);
}

template <>
void applyGain<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.applyGain1(gain, input, output, size);
}

template <>
void applyGain<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.applyGain(gain, input, output, size);
}

template <>
void divide<float>(const float* input, const float* divisor, float* output, unsigned size) noexcept
{
    return simdDispatch.divide(input, divisor, output, size);
}

template <>
void multiplyAdd<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.multiplyAdd(gain, input, output, size);
}

template <>
void multiplyAdd<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.multiplyAdd1(gain, input, output, size);
}

template <>
float linearRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    return simdDispatch.linearRamp(output, start, step, size);
}

template <>
float multiplicativeRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    return simdDispatch.multiplicativeRamp(output, start, step, size);
}

template <>
void add<float>(const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.add(input, output, size);
}

template <>
void add<float>(float value, float* output, unsigned size) noexcept
{
    return simdDispatch.add1(value, output, size);
}

template <>
void subtract<float>(const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.subtract(input, output, size);
}

template <>
void subtract<float>(float value, float* output, unsigned size) noexcept
{
    return simdDispatch.subtract1(value, output, size);
}

template <>
void copy<float>(const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.copy(input, output, size);
}

template <>
float mean<float>(const float* vector, unsigned size) noexcept
{
    return simdDispatch.mean(vector, size);
}

template <>
float meanSquared<float>(const float* vector, unsigned size) noexcept
{
    return simdDispatch.meanSquared(vector, size);
}

template <>
void cumsum<float>(const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.cumsum(input, output, size);
}

template <>
void diff<float>(const float* input, float* output, unsigned size) noexcept
{
    return simdDispatch.diff(input, output, size);
}

///

static cpuid::cpuinfo& cpuInfo()
{
    static cpuid::cpuinfo info;
    return info;
}

template <class T>
bool SIMDDispatch<T>::getStatus(SIMDOps op) const
{
    const unsigned index = static_cast<unsigned>(op);
    ASSERT(index < simdStatus.size());
    return simdStatus[index];
}

template <class T>
void SIMDDispatch<T>::setStatus(SIMDOps op, bool enable)
{
    const unsigned index = static_cast<unsigned>(op);
    ASSERT(index < simdStatus.size());

    simdStatus[index] = enable;

    const cpuid::cpuinfo& info = cpuInfo();
    bool useSSE = enable && info.has_sse();
    bool useAVX = enable && info.has_avx();

    switch (op) {
        default:
            break;

        case SIMDOps::writeInterleaved:
            if (useSSE)
                writeInterleaved = &writeInterleavedSSE;
            else
                writeInterleaved = &writeInterleavedScalar<float>;
            break;

        case SIMDOps::readInterleaved:
            if (useSSE)
                readInterleaved = &readInterleavedSSE;
            else
                readInterleaved = &readInterleavedScalar<float>;
            break;

        case SIMDOps::gain:
            if (useAVX) {
                applyGain = &applyGainAVX;
                applyGain1 = &applyGainAVX;
            }
            else if (useSSE) {
                applyGain = &applyGainSSE;
                applyGain1 = &applyGainSSE;
            }
            else {
                applyGain = &applyGainScalar<float>;
                applyGain1 = &applyGainScalar<float>;
            }
            break;

        case SIMDOps::divide:
            if (useSSE)
                divide = &divideSSE;
            else
                divide = &divideScalar<float>;
            break;

        case SIMDOps::multiplyAdd:
            if (useSSE) {
                multiplyAdd = &multiplyAddSSE;
                multiplyAdd1 = &multiplyAddSSE;
            }
            else {
                multiplyAdd = &multiplyAddScalar<float>;
                multiplyAdd1 = &multiplyAddScalar<float>;
            }
            break;

        case SIMDOps::linearRamp:
            if (useSSE)
                linearRamp = &linearRampSSE;
            else
                linearRamp = &linearRampScalar<float>;
            break;

        case SIMDOps::multiplicativeRamp:
            if (useSSE)
                multiplicativeRamp = &multiplicativeRampSSE;
            else
                multiplicativeRamp = &multiplicativeRampScalar<float>;
            break;

        case SIMDOps::add:
            if (useSSE) {
                add = &addSSE;
                add1 = &addSSE;
            }
            else {
                add = &addScalar<float>;
                add1 = &addScalar<float>;
            }
            break;

        case SIMDOps::subtract:
            if (useSSE) {
                subtract = &subtractSSE;
                subtract1 = &subtractSSE;
            }
            else {
                subtract = &subtractScalar<float>;
                subtract1 = &subtractScalar<float>;
            }
            break;

        case SIMDOps::copy:
            if (useSSE)
                copy = &copySSE;
            else
                copy = &copyScalar<float>;
            break;

        case SIMDOps::cumsum:
            if (useSSE)
                cumsum = &cumsumSSE;
            else
                cumsum = &cumsumScalar<float>;
            break;

        case SIMDOps::diff:
            if (useSSE)
                diff = &diffSSE;
            else
                diff = &diffScalar<float>;
            break;

        case SIMDOps::mean:
            if (useSSE)
                mean = &meanSSE;
            else
                mean = &meanScalar<float>;
            break;

        case SIMDOps::meanSquared:
            if (useSSE)
                meanSquared = &meanSquaredSSE;
            else
                meanSquared = &meanSquaredScalar<float>;
            break;
    }
}

template <class T>
void SIMDDispatch<T>::resetStatus()
{
    setStatus(SIMDOps::writeInterleaved, false);
    setStatus(SIMDOps::readInterleaved, false);
    setStatus(SIMDOps::fill, true);
    setStatus(SIMDOps::gain, true);
    setStatus(SIMDOps::divide, false);
    setStatus(SIMDOps::linearRamp, false);
    setStatus(SIMDOps::multiplicativeRamp, true);
    setStatus(SIMDOps::add, false);
    setStatus(SIMDOps::subtract, false);
    setStatus(SIMDOps::multiplyAdd, false);
    setStatus(SIMDOps::copy, false);
    setStatus(SIMDOps::cumsum, true);
    setStatus(SIMDOps::diff, false);
    setStatus(SIMDOps::sfzInterpolationCast, true);
    setStatus(SIMDOps::mean, false);
    setStatus(SIMDOps::meanSquared, false);
    setStatus(SIMDOps::upsampling, true);
}

///

static volatile bool simdInitialized = false;
static std::mutex simdMutex;

SIMDInitializer::SIMDInitializer()
{
    std::lock_guard<std::mutex> lock { simdMutex };

    if (!simdInitialized) {
        simdDispatch.resetStatus();
        simdInitialized = true;
    }
}

}
