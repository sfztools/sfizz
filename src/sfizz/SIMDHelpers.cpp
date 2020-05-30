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
    simdStatus[static_cast<unsigned>(SIMDOps::writeInterleaved)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::readInterleaved)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::fill)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::gain)] = false;
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

template<>
void applyGain<float>(float gain, const float* input, float* output, unsigned size) noexcept
{
    const auto sentinel = output + size;

    if (getSIMDOpStatus(SIMDOps::gain)) {
#if SFIZZ_CPU_FAMILY_X86_64 || SFIZZ_CPU_FAMILY_I386
        if (cpuInfo.has_sse()) {
            const auto* lastAligned = prevAligned(sentinel);
            const auto mmGain = _mm_set_ps1(gain);
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

template<>
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

}
