// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "DataHelpers.h"
#include "sfizz/Synth.h"
#include "sfizz/LFO.h"
#include "sfizz/Region.h"
#include "catch2/catch.hpp"

static bool computeLFO(DataPoints& dp, const fs::path& sfzPath, double sampleRate, size_t numFrames)
{
    sfz::Synth synth;
    sfz::Resources& resources = synth.getResources();

    if (!synth.loadSfzFile(sfzPath))
        return false;

    if (synth.getNumRegions() != 1)
        return false;

    size_t bufferSize = static_cast<size_t>(synth.getSamplesPerBlock());

    const std::vector<sfz::LFODescription>& desc = synth.getRegionView(0)->lfos;
    size_t numLfos = desc.size();
    std::vector<std::unique_ptr<sfz::LFO>> lfos(numLfos);

    for (size_t l = 0; l < numLfos; ++l) {
        sfz::LFO* lfo = new sfz::LFO(resources);
        lfos[l].reset(lfo);
        lfo->setSampleRate(sampleRate);
        lfo->configure(&desc[l]);
    }

    std::vector<float> outputMemory(numLfos * numFrames);

    for (size_t l = 0; l < numLfos; ++l) {
        lfos[l]->start(0);
    }

    std::vector<absl::Span<float>> lfoOutputs(numLfos);
    for (size_t l = 0; l < numLfos; ++l) {
        lfoOutputs[l] = absl::MakeSpan(&outputMemory[l * numFrames], numFrames);
        for (size_t i = 0, currentFrames; i < numFrames; i += currentFrames) {
            currentFrames = std::min(numFrames - i, bufferSize);
            lfos[l]->process(lfoOutputs[l].subspan(i, currentFrames));
        }
    }

    dp.rows = numFrames;
    dp.cols = numLfos + 1;
    dp.data.reset(new float[dp.rows * dp.cols]);

    for (size_t i = 0; i < numFrames; ++i) {
        dp(i, 0) = i / sampleRate;
        for (size_t l = 0; l < numLfos; ++l)
            dp(i, 1 + l) = lfoOutputs[l][i];
    }

    return true;
}

double meanSquareError(const float* a, const float* b, size_t count, size_t step)
{
    double sum = 0;
    for (size_t i = 0; i < count; ++i) {
        double diff = a[i * step] - b[i * step];
        sum += diff * diff;
    }
    return sum / count;
}

static constexpr double mseThreshold = 1e-3;

TEST_CASE("[LFO] Waves")
{
    DataPoints ref;
    REQUIRE(load_txt_file(ref, "tests/lfo/lfo_waves_reference.dat"));

    DataPoints cur;
    REQUIRE(computeLFO(cur, "tests/lfo/lfo_waves.sfz", 100.0, ref.rows));

    REQUIRE(ref.rows == cur.rows);
    REQUIRE(ref.cols == cur.cols);

    for (size_t l = 1; l < cur.cols; ++l) {
        double mse = meanSquareError(&ref.data[l], &cur.data[l], ref.rows, ref.cols);
        REQUIRE(mse < mseThreshold);
    }
}

TEST_CASE("[LFO] Subwave")
{
    DataPoints ref;
    REQUIRE(load_txt_file(ref, "tests/lfo/lfo_subwave_reference.dat"));

    DataPoints cur;
    REQUIRE(computeLFO(cur, "tests/lfo/lfo_subwave.sfz", 100.0, ref.rows));

    REQUIRE(ref.rows == cur.rows);
    REQUIRE(ref.cols == cur.cols);

    for (size_t l = 1; l < cur.cols; ++l) {
        double mse = meanSquareError(&ref.data[l], &cur.data[l], ref.rows, ref.cols);
        REQUIRE(mse < mseThreshold);
    }
}

TEST_CASE("[LFO] Fade and delay")
{
    DataPoints ref;
    REQUIRE(load_txt_file(ref, "tests/lfo/lfo_fade_and_delay_reference.dat"));

    DataPoints cur;
    REQUIRE(computeLFO(cur, "tests/lfo/lfo_fade_and_delay.sfz", 100.0, ref.rows));

    REQUIRE(ref.rows == cur.rows);
    REQUIRE(ref.cols == cur.cols);

    for (size_t l = 1; l < cur.cols; ++l) {
        double mse = meanSquareError(&ref.data[l], &cur.data[l], ref.rows, ref.cols);
        REQUIRE(mse < mseThreshold);
    }
}
