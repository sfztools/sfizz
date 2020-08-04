// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
   This program generates the data file of a LFO output recorded for a fixed
   duration. The file contains columns for each LFO in the SFZ region.
   The columns are: Time, Lfo1, ... LfoN
   One can use Gnuplot to display this data.
   Example:
     sfizz_plot_lfo file.sfz > lfo.dat
     gnuplot
     plot "lfo.dat" using 1:2 with lines
 */

#include "sfizz/Synth.h"
#include "sfizz/LFO.h"
#include "sfizz/LFODescription.h"
#include <absl/types/span.h>
#include <vector>
#include <iostream>
#include <cmath>

//==============================================================================

static constexpr double sampleRate = 44100.0; // sample rate used to compute
static constexpr double duration = 5.0; // length in seconds

/**
   Print usage information
 */
static void usage()
{
    std::cerr << "Usage: sfizz_plot_lfo <file.sfz>" "\n";
}

static std::vector<sfz::LFODescription> lfoDescriptionFromSfzFile(const fs::path &sfzPath, bool &success)
{
    sfz::Synth synth;

    if (!synth.loadSfzFile(sfzPath)) {
        std::cerr << "Cannot load the SFZ file.\n";
        success = false;
        return {};
    }

    if (synth.getNumRegions() != 1) {
        std::cerr << "The SFZ file must contain exactly one region.\n";
        success = false;
        return {};
    }

    success = true;
    return synth.getRegionView(0)->lfos;
}

/**
   Program which loads LFO configuration and generates plot data for the given duration.
 */
int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 2) {
        usage();
        return 1;
    }

    fs::path sfzPath = argv[1];
    bool success = false;
    const std::vector<sfz::LFODescription> desc = lfoDescriptionFromSfzFile(sfzPath, success);
    if (!success)
        return 1;

    size_t numLfos = desc.size();
    std::vector<sfz::LFO> lfos(numLfos);

    for (size_t l = 0; l < numLfos; ++l) {
        lfos[l].setSampleRate(sampleRate);
        lfos[l].configure(&desc[l]);
    }

    size_t numFrames = (size_t)std::ceil(sampleRate * duration);
    std::vector<float> outputMemory(numLfos * numFrames);

    std::vector<absl::Span<float>> lfoOutputs(numLfos);
    for (size_t l = 0; l < numLfos; ++l) {
        lfoOutputs[l] = absl::MakeSpan(&outputMemory[l * numFrames], numFrames);
        lfos[l].process(lfoOutputs[l]);
    }

    for (size_t i = 0; i < numFrames; ++i) {
        std::cout << (i / sampleRate);
        for (size_t l = 0; l < numLfos; ++l)
            std::cout << ' ' << lfoOutputs[l][i];
        std::cout << '\n';
    }

    return 0;
}
