// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Oversampler.h"
#include "hiir/Upsampler2x4Sse.h"

template<>
inline void sfz::upsample2xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2x4Sse<sfz::coeffsStage2x.size()> upsampler;
    upsampler.set_coefs(sfz::coeffsStage2x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}
template<>
inline void sfz::upsample4xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2x4Sse<sfz::coeffsStage4x.size()> upsampler;
    upsampler.set_coefs(sfz::coeffsStage4x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}
template<>
inline void sfz::upsample8xStage<true>(absl::Span<const float> input, absl::Span<float> output)
{
    ASSERT(output.size() >= 2 * input.size());
    hiir::Upsampler2x4Sse<sfz::coeffsStage8x.size()> upsampler;
    upsampler.set_coefs(sfz::coeffsStage8x.data());
    upsampler.process_block(output.data(), input.data(), input.size());
}
