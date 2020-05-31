#include "SIMDHelpers.h"
#include <array>
#include "cpuid/cpuinfo.hpp"

#include "SIMDConfig.h"

#if SFIZZ_HAVE_SSE2
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

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
    simdStatus[static_cast<unsigned>(SIMDOps::mathfuns)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::loopingSFZIndex)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::saturatingSFZIndex)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::linearRamp)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplicativeRamp)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::add)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::subtract)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplyAdd)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::copy)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::pan)] = false;
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

constexpr uintptr_t TypeAlignment = 4;

template <class T>
inline void tickRead(const T*& input, T*& outputLeft, T*& outputRight)
{
    *outputLeft++ = *input++;
    *outputRight++ = *input++;
}

template <class T>
inline void tickWrite(T*& output, const T*& inputLeft, const T*& inputRight)
{
    *output++ = *inputLeft++;
    *output++ = *inputRight++;
}

void readInterleaved(const float* input, float* outputLeft, float* outputRight, unsigned inputSize) noexcept
{
    const auto sentinel = input + inputSize - 1;

    if (getSIMDOpStatus(SIMDOps::readInterleaved)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(input + inputSize - 4);
            while (unaligned(input, outputLeft, outputRight) && input < lastAligned)
                tickRead(input, outputLeft, outputRight);

            while (input < lastAligned) {
                auto register0 = _mm_load_ps(input);
                auto register1 = _mm_load_ps(input + 4);
                auto register2 = register0;
                // register 2 holds the copy of register 0 that is going to get erased by the first operation
                // Remember that the bit mask reads from the end; 10 00 10 00 means
                // "take 0 from a, take 2 from a, take 0 from b, take 2 from b"
                register0 = _mm_shuffle_ps(register0, register1, 0b10001000);
                register1 = _mm_shuffle_ps(register2, register1, 0b11011101);
                _mm_store_ps(outputLeft, register0);
                _mm_store_ps(outputRight, register1);
                incrementAll<4>(input, input, outputLeft, outputRight);
            }
            // Fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (input < sentinel)
        tickRead(input, outputLeft, outputRight);
}

void writeInterleaved(const float* inputLeft, const float* inputRight, float* output, unsigned outputSize) noexcept
{
    const auto sentinel = output + outputSize - 1;

    if (getSIMDOpStatus(SIMDOps::writeInterleaved)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(output + outputSize - 4);

            while (unaligned(output, inputRight, inputLeft) && output < lastAligned)
                tickWrite(output, inputLeft, inputRight);

            while (output < lastAligned) {
                const auto lInRegister = _mm_load_ps(inputLeft);
                const auto rInRegister = _mm_load_ps(inputRight);
                const auto outRegister1 = _mm_unpacklo_ps(lInRegister, rInRegister);
                _mm_store_ps(output, outRegister1);
                const auto outRegister2 = _mm_unpackhi_ps(lInRegister, rInRegister);
                _mm_store_ps(output + 4, outRegister2);
                incrementAll<4>(output, output, inputLeft, inputRight);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        tickWrite(output, inputLeft, inputRight);
}

template <>
void applyGain<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::gain)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            const auto mmGain = _mm_set1_ps(gain);
            while (unaligned(input, output) && output < lastAligned)
                *output++ = gain * (*input++);

            while (output < lastAligned) {
                _mm_store_ps(output, _mm_mul_ps(mmGain, _mm_load_ps(input)));
                incrementAll<4>(input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ = gain * (*input++);
}

template <>
void applyGain<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::gain)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);

            while (unaligned(input, output) && output < lastAligned)
                *output++ = (*gain++) * (*input++);

            while (output < lastAligned) {
                _mm_store_ps(output, _mm_mul_ps(_mm_load_ps(gain), _mm_load_ps(input)));
                incrementAll<4>(gain, input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ = (*gain++) * (*input++);
}

template <>
void divide<float>(const float* input, const float* divisor, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::divide)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);

            while (unaligned(input, output) && output < lastAligned)
                *output++ = (*input++) / (*divisor++);

            while (output < lastAligned) {
                _mm_store_ps(output, _mm_div_ps(_mm_load_ps(input), _mm_load_ps(divisor)));
                incrementAll<4>(divisor, input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ = (*input++) / (*divisor++);
}

template <>
void multiplyAdd<float>(const float* gain, const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::multiplyAdd)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            while (unaligned(input, output) && output < lastAligned)
                *output++ += (*gain++) * (*input++);

            while (output < lastAligned) {
                auto mmOut = _mm_load_ps(output);
                mmOut = _mm_add_ps(_mm_mul_ps(_mm_load_ps(gain), _mm_load_ps(input)), mmOut);
                _mm_store_ps(output, mmOut);
                incrementAll<4>(gain, input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ += (*gain++) * (*input++);
}

template <>
void multiplyAdd<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::multiplyAdd)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            while (unaligned(input, output) && output < lastAligned)
                *output++ += gain * (*input++);

            auto mmGain = _mm_set1_ps(gain);
            while (output < lastAligned) {
                auto mmOut = _mm_load_ps(output);
                mmOut = _mm_add_ps(_mm_mul_ps(mmGain, _mm_load_ps(input)), mmOut);
                _mm_store_ps(output, mmOut);
                incrementAll<4>(input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ += gain * (*input++);
}

template <>
float linearRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::linearRamp)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            while (unaligned(output) && output < lastAligned) {
                *output++ = start;
                start += step;
            }

            auto mmStart = _mm_set1_ps(start - step);
            auto mmStep = _mm_set_ps(step + step + step + step, step + step + step, step + step, step);
            while (output < lastAligned) {
                mmStart = _mm_add_ps(mmStart, mmStep);
                _mm_store_ps(output, mmStart);
                mmStart = _mm_shuffle_ps(mmStart, mmStart, _MM_SHUFFLE(3, 3, 3, 3));
                incrementAll<4>(output);
            }
            start = _mm_cvtss_f32(mmStart) + step;
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel) {
        *output++ = start;
        start += step;
    }
    return start;
}

template <>
float multiplicativeRamp<float>(float* output, float start, float step, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::linearRamp)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            while (unaligned(output) && output < lastAligned) {
                *output++ = start;
                start *= step;
            }

            auto mmStart = _mm_set1_ps(start / step);
            auto mmStep = _mm_set_ps(step * step * step * step, step * step * step, step * step, step);
            while (output < lastAligned) {
                mmStart = _mm_mul_ps(mmStart, mmStep);
                _mm_store_ps(output, mmStart);
                mmStart = _mm_shuffle_ps(mmStart, mmStart, _MM_SHUFFLE(3, 3, 3, 3));
                incrementAll<4>(output);
            }
            start = _mm_cvtss_f32(mmStart) * step;
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel) {
        *output++ = start;
        start *= step;
    }
    return start;
}

template <>
void add<float>(const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::add)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);

            while (unaligned(input, output) && output < lastAligned)
                *output++ += *input++;

            while (output < lastAligned) {
                _mm_store_ps(output, _mm_add_ps(_mm_load_ps(output), _mm_load_ps(input)));
                incrementAll<4>(input, output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ += *input++;
}

template <>
void add<float>(float value, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::add)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);

            while (unaligned(output) && output < lastAligned)
                *output++ += value;

            const auto mmValue = _mm_set1_ps(value);
            while (output < lastAligned) {
                _mm_store_ps(output, _mm_add_ps(_mm_load_ps(output), mmValue));
                incrementAll<4>(output);
            }
            // fallthrough from lastAligned to sentinel
        }
#endif
    }

    while (output < sentinel)
        *output++ += value;
}

}