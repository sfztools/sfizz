// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDHelpers.h"
#include <array>
#include "cpuid/cpuinfo.hpp"
#include "simd/HelpersSSE.h"
#include "simd/HelpersAVX.h"
#include "SIMDConfig.h"

namespace sfz {

static std::array<bool, static_cast<unsigned>(SIMDOps::_sentinel)> simdStatus;
static bool simdStatusInitialized = false;
static cpuid::cpuinfo cpuInfo;

void resetSIMDStatus()
{
    simdStatus[static_cast<unsigned>(SIMDOps::writeInterleaved)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::readInterleaved)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::fill)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::gain)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::divide)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::linearRamp)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplicativeRamp)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::add)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::subtract)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplyAdd)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::copy)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::cumsum)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::diff)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::sfzInterpolationCast)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::mean)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::meanSquared)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::upsampling)] = true;
    simdStatusInitialized = true;
}

void setSIMDOpStatus(SIMDOps op, bool status)
{
    if (!simdStatusInitialized)
        resetSIMDStatus();

    simdStatus[static_cast<unsigned>(op)] = status;
}

bool getSIMDOpStatus(SIMDOps op)
{
    if (!simdStatusInitialized)
        resetSIMDStatus();

    return simdStatus[static_cast<unsigned>(op)];
}

void readInterleaved(const float* input, float* outputLeft, float* outputRight, unsigned inputSize) noexcept
{
    if (getSIMDOpStatus(SIMDOps::readInterleaved)) {
        if (cpuInfo.has_sse())
            return readInterleavedSSE(input, outputLeft, outputRight, inputSize);
    }
    return readInterleavedScalar(input, outputLeft, outputRight, inputSize);
}

void writeInterleaved(const float* inputLeft, const float* inputRight, float* output, unsigned outputSize) noexcept
{
    if (getSIMDOpStatus(SIMDOps::writeInterleaved)) {
        if (cpuInfo.has_sse())
            return writeInterleavedSSE(inputLeft, inputRight, output, outputSize);
    }
    return writeInterleavedScalar(inputLeft, inputRight, output, outputSize);
}

template <>
void applyGain<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::gain)) {
        if (cpuInfo.has_avx())
            return applyGainAVX(gain, input, output, size);
        else if (cpuInfo.has_sse())
            return applyGainSSE(gain, input, output, size);
    }
    return applyGainScalar(gain, input, output, size);
}

template <>
void applyGain<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::gain)) {
        if (cpuInfo.has_avx())
            return applyGainAVX(gain, input, output, size);
        else if (cpuInfo.has_sse())
            return applyGainSSE(gain, input, output, size);
    }
    return applyGainScalar(gain, input, output, size);
}

template <>
void divide<float>(const float* input, const float* divisor, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::divide)) {
        if (cpuInfo.has_sse())
            return divideSSE(input, divisor, output, size);
    }
    return divideScalar(input, divisor, output, size);
}

template <>
void multiplyAdd<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::multiplyAdd)) {
        if (cpuInfo.has_sse())
            return multiplyAddSSE(gain, input, output, size);
    }
    return multiplyAddScalar(gain, input, output, size);
}

template <>
void multiplyAdd<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::multiplyAdd)) {
        if (cpuInfo.has_sse())
            return multiplyAddSSE(gain, input, output, size);
    }
    return multiplyAddScalar(gain, input, output, size);
}

template <>
float linearRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::linearRamp)) {
        if (cpuInfo.has_sse())
            return linearRampSSE(output, start, step, size);
    }
    return linearRampScalar(output, start, step, size);
}

template <>
float multiplicativeRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::multiplicativeRamp)) {
        if (cpuInfo.has_sse())
            return multiplicativeRampSSE(output, start, step, size);
    }
    return multiplicativeRampScalar(output, start, step, size);
}

template <>
void add<float>(const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::add)) {
        if (cpuInfo.has_sse())
            return addSSE(input, output, size);
    }
    return addScalar(input, output, size);
}

template <>
void add<float>(float value, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::add)) {
        if (cpuInfo.has_sse())
            return addSSE(value, output, size);
    }
    return addScalar(value, output, size);
}

template <>
void subtract<float>(const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::subtract)) {
        if (cpuInfo.has_sse())
            return subtractSSE(input, output, size);
    }
    return subtractScalar(input, output, size);
}

template <>
void subtract<float>(float value, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::subtract)) {
        if (cpuInfo.has_sse())
            return subtractSSE(value, output, size);
    }
    return subtractScalar(value, output, size);
}

template <>
void copy<float>(const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::copy)) {
        if (cpuInfo.has_sse())
            return copySSE(input, output, size);
    }
    std::copy(input, input + size, output);
}

template <>
float mean<float>(const float* vector, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::mean)) {
        if (cpuInfo.has_sse())
            return meanSSE(vector, size);
    }
    return meanScalar(vector, size);
}

template <>
float meanSquared<float>(const float* vector, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::meanSquared)) {
        if (cpuInfo.has_sse())
            return meanSquaredSSE(vector, size);
    }
    return meanSquaredScalar(vector, size);
}

template <>
void cumsum<float>(const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::cumsum)) {
        if (cpuInfo.has_sse())
            return cumsumSSE(input, output, size);
    }
    return cumsumScalar(input, output, size);
}

template <>
void diff<float>(const float* input, float* output, unsigned size) noexcept
{
    if (getSIMDOpStatus(SIMDOps::diff)) {
        if (cpuInfo.has_sse())
            return diffSSE(input, output, size);
    }
    return diffScalar(input, output, size);
}

}
