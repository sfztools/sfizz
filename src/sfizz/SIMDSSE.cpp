// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDConfig.h"

#if SFIZZ_HAVE_SSE2

#include "SIMDHelpers.h"
#include <array>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "mathfuns/sse_mathfun.h"

constexpr uintptr_t TypeAlignment = 4;

template <>
void sfz::applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    const auto size = std::min(output.size(), input.size());
    const auto* lastAligned = prevAligned(output.begin() + size);
    const auto mmGain = _mm_set_ps1(gain);

    while (unaligned(out, in) && out < lastAligned)
        *out++ = gain * (*in++);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_mul_ps(mmGain, _mm_load_ps(in)));
        incrementAll<TypeAlignment>(out, in);
    }

    while (out < output.end())
        *out++ = gain * (*in++);
}

template <>
void sfz::applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    auto* g = gain.begin();
    const auto size = std::min(output.size(), std::min(input.size(), gain.size()));
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in, g) && out < lastAligned)
        _internals::snippetGainSpan<float>(g, in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_mul_ps(_mm_load_ps(g), _mm_load_ps(in)));
        incrementAll<TypeAlignment>(g, in, out);
    }

    while (out < output.end())
        _internals::snippetGainSpan<float>(g, in, out);
}


template <>
void sfz::divide<float, true>(absl::Span<const float> input, absl::Span<const float> divisor, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    auto* div = divisor.begin();
    const auto size = std::min(output.size(), std::min(input.size(), divisor.size()));
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in, div) && out < lastAligned)
        _internals::snippetDivSpan<float>(in, div, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_div_ps(_mm_load_ps(in), _mm_load_ps(div)));
        incrementAll<TypeAlignment>(in, div, out);
    }

    while (out < output.end())
        _internals::snippetDivSpan<float>(in, div, out);
}


template <>
void sfz::multiplyAdd<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    auto* g = gain.begin();
    const auto size = std::min(output.size(), std::min(input.size(), gain.size()));
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in, g) && out < lastAligned)
        _internals::snippetMultiplyAdd<float>(g, in, out);

    while (out < lastAligned) {
        auto mmOut = _mm_load_ps(out);
        mmOut = _mm_add_ps(_mm_mul_ps(_mm_load_ps(g), _mm_load_ps(in)), mmOut);
        _mm_store_ps(out, mmOut);
        incrementAll<TypeAlignment>(g, in, out);
    }

    while (out < output.end())
        _internals::snippetMultiplyAdd<float>(g, in, out);
}

template <>
void sfz::multiplyAdd<float, true>(const float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    auto* in = input.begin();
    auto* out = output.begin();
    const auto size = std::min(output.size(), input.size());
    const auto* lastAligned = prevAligned(output.begin() + size);

    while (unaligned(out, in) && out < lastAligned)
        _internals::snippetMultiplyAdd<float>(gain, in, out);

    auto mmGain = _mm_set1_ps(gain);
    while (out < lastAligned) {
        auto mmOut = _mm_load_ps(out);
        mmOut = _mm_add_ps(_mm_mul_ps(mmGain, _mm_load_ps(in)), mmOut);
        _mm_store_ps(out, mmOut);
        incrementAll<TypeAlignment>(in, out);
    }

    while (out < output.end())
        _internals::snippetMultiplyAdd<float>(gain, in, out);
}

template <>
float sfz::linearRamp<float, true>(absl::Span<float> output, float value, float step) noexcept
{
    auto* out = output.begin();
    const auto* lastAligned = prevAligned(output.end());

    while (unaligned(out) && out < lastAligned)
        _internals::snippetRampLinear<float>(out, value, step);

    auto mmValue = _mm_set1_ps(value - step);
    auto mmStep = _mm_set_ps(step + step + step + step, step + step + step, step + step, step);

    while (out < lastAligned) {
        mmValue = _mm_add_ps(mmValue, mmStep);
        _mm_store_ps(out, mmValue);
        mmValue = _mm_shuffle_ps(mmValue, mmValue, _MM_SHUFFLE(3, 3, 3, 3));
        out += TypeAlignment;
    }

    value = _mm_cvtss_f32(mmValue) + step;

    while (out < output.end())
        _internals::snippetRampLinear<float>(out, value, step);
    return value;
}

template <>
float sfz::multiplicativeRamp<float, true>(absl::Span<float> output, float value, float step) noexcept
{
    auto* out = output.begin();
    const auto* lastAligned = prevAligned(output.end());

    while (unaligned(out) && out < lastAligned)
        _internals::snippetRampMultiplicative<float>(out, value, step);

    auto mmValue = _mm_set1_ps(value / step);
    auto mmStep = _mm_set_ps(step * step * step * step, step * step * step, step * step, step);

    while (out < lastAligned) {
        mmValue = _mm_mul_ps(mmValue, mmStep);
        _mm_store_ps(out, mmValue);
        mmValue = _mm_shuffle_ps(mmValue, mmValue, _MM_SHUFFLE(3, 3, 3, 3));
        out += TypeAlignment;
    }

    value = _mm_cvtss_f32(mmValue) * step;
    while (out < output.end())
        _internals::snippetRampMultiplicative<float>(out, value, step);
    return value;
}

template <>
void sfz::add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && out < lastAligned)
        _internals::snippetAdd<float>(in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_add_ps(_mm_load_ps(in), _mm_load_ps(out)));
        incrementAll<TypeAlignment>(in, out);
    }

    while (out < sentinel)
        _internals::snippetAdd<float>(in, out);
}

template <>
void sfz::add<float, true>(float value, absl::Span<float> output) noexcept
{
    auto* out = output.begin();
    auto* sentinel = output.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(out) && out < lastAligned)
        _internals::snippetAdd<float>(value, out);

    auto mmValue = _mm_set_ps1(value);
    while (out < lastAligned) {
        _mm_store_ps(out, _mm_add_ps(mmValue, _mm_load_ps(out)));
        out += TypeAlignment;
    }

    while (out < sentinel)
        _internals::snippetAdd<float>(value, out);
}

template <>
void sfz::subtract<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && out < lastAligned)
        _internals::snippetSubtract<float>(in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_sub_ps(_mm_load_ps(out), _mm_load_ps(in)));
        incrementAll<TypeAlignment>(in, out);
    }

    while (out < sentinel)
        _internals::snippetSubtract<float>(in, out);
}

template <>
void sfz::subtract<float, true>(const float value, absl::Span<float> output) noexcept
{
    auto* out = output.begin();
    auto* sentinel = output.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(out) && out < lastAligned)
        _internals::snippetSubtract<float>(value, out);

    auto mmValue = _mm_set_ps1(value);
    while (out < lastAligned) {
        _mm_store_ps(out, _mm_sub_ps(_mm_load_ps(out), mmValue));
        out += TypeAlignment;
    }

    while (out < sentinel)
        _internals::snippetSubtract<float>(value, out);
}

template <>
void sfz::copy<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    auto* in = input.begin();
    auto* out = output.begin();
    auto* sentinel = out + min(input.size(), output.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(in, out) && out < lastAligned)
        _internals::snippetCopy<float>(in, out);

    while (out < lastAligned) {
        _mm_store_ps(out, _mm_load_ps(in));
        incrementAll<TypeAlignment>(in, out);
    }

    while (out < sentinel)
        _internals::snippetCopy<float>(in, out);
}

template <>
void sfz::pan<float, true>(absl::Span<const float> panEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= panEnvelope.size());
    CHECK(rightBuffer.size() >= panEnvelope.size());
    auto* pan = panEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = pan + min(panEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(pan, left, right) && pan < lastAligned) {
        _internals::snippetPan(*pan, *left, *right);
        incrementAll(pan, left, right);
    }

    const auto mmOne = _mm_set_ps1(1.0f);
    const auto mmPiFour = _mm_set_ps1(piFour<float>());
    __m128 mmCos;
    __m128 mmSin;
    while (pan < lastAligned) {
        auto mmPan = _mm_load_ps(pan);
        mmPan = _mm_add_ps(mmOne, mmPan);
        mmPan = _mm_mul_ps(mmPan, mmPiFour);
        sincos_ps(mmPan, &mmSin, &mmCos);
        auto mmLeft = _mm_mul_ps(mmCos, _mm_load_ps(left));
        auto mmRight = _mm_mul_ps(mmSin, _mm_load_ps(right));
        _mm_store_ps(left, mmLeft);
        _mm_store_ps(right, mmRight);
        incrementAll<TypeAlignment>(pan, left, right);
    }

    while (pan < sentinel){
        _internals::snippetPan(*pan, *left, *right);
        incrementAll(pan, left, right);
    }
}

template <>
void sfz::width<float, true>(absl::Span<const float> widthEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= widthEnvelope.size());
    CHECK(rightBuffer.size() >= widthEnvelope.size());
    auto* width = widthEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = width + min(widthEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(width, left, right) && width < lastAligned) {
        _internals::snippetWidth(*width, *left, *right);
        incrementAll(width, left, right);
    }

    const auto mmPiFour = _mm_set_ps1(piFour<float>());
    __m128 mmCos;
    __m128 mmSin;
    while (width < lastAligned) {
        auto mmWidth = _mm_load_ps(width);
        mmWidth = _mm_mul_ps(mmWidth, mmPiFour);
        sincos_ps(mmWidth, &mmSin, &mmCos);
        auto mmCosPlusSine = _mm_add_ps(mmCos, mmSin);
        auto mmCosMinusSine = _mm_sub_ps(mmCos, mmSin);
        auto mmLeft = _mm_load_ps(left);
        auto mmRight = _mm_load_ps(right);
        auto mmTemp = _mm_mul_ps(mmCosMinusSine, mmRight);
        mmRight = _mm_add_ps(_mm_mul_ps(mmCosMinusSine, mmLeft), _mm_mul_ps(mmCosPlusSine, mmRight));
        mmLeft = _mm_add_ps(_mm_mul_ps(mmCosPlusSine, mmLeft), mmTemp);
        _mm_store_ps(left, mmLeft);
        _mm_store_ps(right, mmRight);
        incrementAll<TypeAlignment>(width, left, right);
    }

    while (width < sentinel){
        _internals::snippetWidth(*width, *left, *right);
        incrementAll(width, left, right);
    }
}

template <>
float sfz::mean<float, true>(absl::Span<const float> vector) noexcept
{
    float result { 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    auto* sentinel = vector.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(value) && value < lastAligned)
        result += *value++;

    auto mmSums = _mm_setzero_ps();
    while (value < lastAligned) {
        mmSums = _mm_add_ps(mmSums, _mm_load_ps(value));
        value += TypeAlignment;
    }

    std::array<float, 4> sseResult;
    _mm_store_ps(sseResult.data(), mmSums);

    for (auto sseValue : sseResult)
        result += sseValue;

    while (value < sentinel)
        result += *value++;

    return result / static_cast<float>(vector.size());
}

template <>
float sfz::meanSquared<float, true>(absl::Span<const float> vector) noexcept
{
    float result { 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    auto* sentinel = vector.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(value) && value < lastAligned) {
        result += (*value) * (*value);
        value++;
    }

    auto mmSums = _mm_setzero_ps();
    while (value < lastAligned) {
        const auto mmValues = _mm_load_ps(value);
        mmSums = _mm_add_ps(mmSums, _mm_mul_ps(mmValues, mmValues));
        value += TypeAlignment;
    }

    std::array<float, 4> sseResult;
    _mm_store_ps(sseResult.data(), mmSums);

    for (auto sseValue : sseResult)
        result += sseValue;

    while (value < sentinel) {
        result += (*value) * (*value);
        value++;
    }

    return result / static_cast<float>(vector.size());
}

template <>
void sfz::cumsum<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());
    const auto lastAligned = prevAligned(sentinel);

    *out++ = *in++;
    while (unaligned(in, out) && in < lastAligned)
        _internals::snippetCumsum(in, out);

    auto mmOutput = _mm_set_ps1(*(out - 1));
    while (in < lastAligned) {
        auto mmOffset = _mm_load_ps(in);
        mmOffset = _mm_add_ps(mmOffset, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOffset), 4)));
        mmOffset = _mm_add_ps(mmOffset, _mm_shuffle_ps(_mm_setzero_ps(), mmOffset, _MM_SHUFFLE(1, 0, 0, 0)));
        mmOutput = _mm_add_ps(mmOutput, mmOffset);
        _mm_store_ps(out, mmOutput);
        mmOutput = _mm_shuffle_ps(mmOutput, mmOutput, _MM_SHUFFLE(3, 3, 3, 3));
        incrementAll<TypeAlignment>(in, out);
    }

    while (in < sentinel)
        _internals::snippetCumsum(in, out);
}

template <>
void sfz::sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> coeffs) noexcept
{
    sfz::sfzInterpolationCast<float, false>(floatJumps, jumps, coeffs);
    // CHECK(jumps.size() >= floatJumps.size());
    // CHECK(jumps.size() == coeffs.size());

    // auto floatJump = floatJumps.data();
    // auto jump = jumps.data();
    // auto coeff = coeffs.data();
    // const auto sentinel = floatJump + min(floatJumps.size(), jumps.size(), coeffs.size());
    // const auto lastAligned = prevAligned(sentinel);

    // while (unaligned(floatJump, reinterpret_cast<float*>(jump), coeff) && floatJump < lastAligned)
    //     _internals::snippetSFZInterpolationCast(floatJump, jump, coeff);

    // while (floatJump < lastAligned) {
    //     auto mmFloatJumps = _mm_load_ps(floatJump);
    //     auto mmIndices = _mm_cvtps_epi32(_mm_sub_ps(mmFloatJumps, _mm_set_ps1(0.4999999552965164184570312f)));
    //     _mm_store_si128(reinterpret_cast<__m128i*>(jump), mmIndices);

    //     auto mmCoeff = _mm_sub_ps(mmFloatJumps, _mm_cvtepi32_ps(mmIndices));
    //     _mm_store_ps(coeff, mmCoeff);
    //     incrementAll<TypeAlignment>(floatJump, jump, coeff);
    // }

    // while(floatJump < sentinel)
    //     _internals::snippetSFZInterpolationCast(floatJump, jump, coeff);
}

template <>
void sfz::diff<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());
    const auto lastAligned = prevAligned(sentinel);

    *out++ = *in++;
    while (unaligned(in, out) && in < lastAligned)
        _internals::snippetDiff(in, out);

    auto mmBase = _mm_set_ps1(*(in - 1));
    while (in < lastAligned) {
        auto mmOutput = _mm_load_ps(in);
        auto mmNextBase = _mm_shuffle_ps(mmOutput, mmOutput, _MM_SHUFFLE(3, 3, 3, 3));
        mmOutput = _mm_sub_ps(mmOutput, mmBase);
        mmBase = mmNextBase;
        mmOutput = _mm_sub_ps(mmOutput, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOutput), 4)));
        _mm_store_ps(out, mmOutput);
        incrementAll<TypeAlignment>(in, out);
    }

    while (in < sentinel)
        _internals::snippetDiff(in, out);
}

#endif // SFIZZ_HAVE_SSE2
