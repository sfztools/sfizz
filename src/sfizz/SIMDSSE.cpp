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
