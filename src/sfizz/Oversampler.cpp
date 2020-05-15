// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Oversampler.h"
#include "Buffer.h"
#include "AudioSpan.h"
#include "SIMDConfig.h"

constexpr std::array<double, 12> coeffsStage2x {
    0.036681502163648017,
    0.13654762463195771,
    0.27463175937945411,
    0.42313861743656667,
    0.56109869787919475,
    0.67754004997416162,
    0.76974183386322659,
    0.83988962484963803,
    0.89226081800387891,
    0.9315419599631839,
    0.96209454837808395,
    0.98781637073289708
};

constexpr std::array<double, 4> coeffsStage4x {
    0.042448989488488006,
    0.17072114107630679,
    0.39329183835224008,
    0.74569514831986694
};

constexpr std::array<double, 3> coeffsStage8x {
    0.055748680811302048,
    0.24305119574153092,
    0.6466991311926823
};


#if SFIZZ_HAVE_SSE
#include "hiir/Upsampler2xSse.h"
using Upsampler2x = hiir::Upsampler2xSse<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xSse<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xSse<coeffsStage8x.size()>;
#elif SFIZZ_HAVE_NEON
#include "hiir/Upsampler2xNeon.h"
using Upsampler2x = hiir::Upsampler2xNeon<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xNeon<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xNeon<coeffsStage8x.size()>;
#else
#include "hiir/Upsampler2xFpu.h"
using Upsampler2x = hiir::Upsampler2xFpu<coeffsStage2x.size()>;
using Upsampler4x = hiir::Upsampler2xFpu<coeffsStage4x.size()>;
using Upsampler8x = hiir::Upsampler2xFpu<coeffsStage8x.size()>;
#endif

sfz::Oversampler::Oversampler(sfz::Oversampling factor, size_t chunkSize)
: factor(factor), chunkSize(chunkSize)
{

}

void sfz::Oversampler::stream(const sfz::AudioBuffer<float>& input, sfz::AudioBuffer<float>& output, std::atomic<size_t>* framesReady)
{
    ASSERT(output.getNumFrames() >= input.getNumFrames() * static_cast<int>(factor));
    ASSERT(output.getNumChannels() == input.getNumChannels());

    const auto numFrames = input.getNumFrames();
    const auto numChannels = input.getNumChannels();

    std::vector<Upsampler2x> upsampler2x (numChannels);
    std::vector<Upsampler4x> upsampler4x (numChannels);
    std::vector<Upsampler8x> upsampler8x (numChannels);

    switch(factor)
    {
    case Oversampling::x8:
        for (auto& upsampler: upsampler8x)
            upsampler.set_coefs(coeffsStage8x.data());
        // fallthrough
    case Oversampling::x4:
        for (auto& upsampler: upsampler4x)
            upsampler.set_coefs(coeffsStage4x.data());
        // fallthrough
    case Oversampling::x2:
        for (auto& upsampler: upsampler2x)
            upsampler.set_coefs(coeffsStage2x.data());
        break;
    case Oversampling::x1:
        for (size_t i = 0; i < numChannels; ++i)
            copy<float>(input.getConstSpan(i), output.getSpan(i).first(numFrames));
        return;
    }

    // Intermediate buffers
    sfz::Buffer<float> buffer1 { chunkSize * 2 };
    sfz::Buffer<float> buffer2 { chunkSize * 4 };
    auto span1 = absl::MakeSpan(buffer1);
    auto span2 = absl::MakeSpan(buffer2);

    size_t inputFrameCounter { 0 };
    size_t outputFrameCounter { 0 };
    while(inputFrameCounter < numFrames)
    {
        // std::cout << "Input frames: " << inputFrameCounter << "/" << numFrames << '\n';
        const auto thisChunkSize = std::min(chunkSize, numFrames - inputFrameCounter);
        const auto outputChunkSize = thisChunkSize * static_cast<int>(factor);
        for (size_t chanIdx = 0; chanIdx < numChannels; chanIdx++) {
            const auto inputChunk = input.getSpan(chanIdx).subspan(inputFrameCounter, thisChunkSize);
            const auto outputChunk = output.getSpan(chanIdx).subspan(outputFrameCounter, outputChunkSize);
            if (factor == Oversampling::x2) {
                upsampler2x[chanIdx].process_block(outputChunk.data(), inputChunk.data(), static_cast<long>(thisChunkSize));
                continue;
            }
            if (factor == Oversampling::x4) {
                upsampler2x[chanIdx].process_block(span1.data(), inputChunk.data(), static_cast<long>(thisChunkSize));
                upsampler4x[chanIdx].process_block(outputChunk.data(), span1.data(), static_cast<long>(thisChunkSize * 2));
                continue;
            }
            else if (factor == Oversampling::x8) {
                upsampler2x[chanIdx].process_block(span1.data(), inputChunk.data(), static_cast<long>(thisChunkSize));
                upsampler4x[chanIdx].process_block(span2.data(), span1.data(), static_cast<long>(thisChunkSize * 2));
                upsampler8x[chanIdx].process_block(outputChunk.data(), span2.data(), static_cast<long>(thisChunkSize * 4));
                continue;
            }
        }
        inputFrameCounter += thisChunkSize;
        outputFrameCounter += outputChunkSize;

        if (framesReady != nullptr)
            framesReady->fetch_add(outputChunkSize);
    }

}
