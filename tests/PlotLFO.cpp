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

#include "sfizz/LFO.h"
#include "sfizz/Parser.h"
#include "absl/strings/numbers.h"
#include "absl/types/span.h"
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
    std::cerr << "Usage: sfizz_plot_lfo <file.sfz>"
              << "\n";
}

/**
   Parser which gets the first region and extracts the configuration of LFO
 */
static void configureLFOwithRegion(
    std::vector<sfz::LFO::Control>& ctls,
    const absl::Span<const sfz::Opcode> opcodes)
{
    for (const sfz::Opcode& opc : opcodes) {
        auto firstParam = opc.firstParameter();
        auto backParam = opc.backParameter();
        if (!firstParam)
            continue;

        uint8_t lfoIndex = *firstParam - 1;
        uint8_t subwaveIndex = backParam ? (*backParam - 1) : 0;

        auto getLfo = [&ctls](uint8_t iLfo) -> sfz::LFO::Control* {
            if (iLfo >= ctls.size())
                ctls.resize(iLfo + 1);
            return &ctls[iLfo];
        };
        auto getSub = [&getLfo](uint8_t iLfo, uint8_t iSub) -> sfz::LFO::Control::Sub* {
            if (iSub >= 8)
                return nullptr;
            sfz::LFO::Control* lfo = getLfo(iLfo);
            lfo->countSubs = std::max<unsigned>(lfo->countSubs, iSub + 1);
            return &lfo->sub[iSub];
        };
        auto getStepSeq = [&getLfo](uint8_t iLfo) -> sfz::LFO::Control::StepSequence* {
            sfz::LFO::Control* lfo = getLfo(iLfo);
            sfz::LFO::Control::StepSequence* seq = lfo->stepSequence.get();
            if (!seq) {
                seq = new sfz::LFO::Control::StepSequence;
                lfo->stepSequence.reset(seq);
            }
            return seq;
        };
        auto getStep = [&getStepSeq](uint8_t iLfo, uint8_t iStep) -> float* {
            if (iStep >= sfz::LFO::Control::StepSequence::maximumSteps)
                return nullptr;
            sfz::LFO::Control::StepSequence* seq = getStepSeq(iLfo);
            if (!seq)
                return nullptr;
            return &seq->steps[iStep];
        };

        switch (opc.lettersOnlyHash) {
        case hash("lfo_freq"): {
            sfz::LFO::Control* ctl = getLfo(lfoIndex);
            if (!absl::SimpleAtof(opc.value, &ctl->freq))
                break;
            break;
        }
        case hash("lfo_phase"): {
            sfz::LFO::Control* ctl = getLfo(lfoIndex);
            if (!absl::SimpleAtof(opc.value, &ctl->phase0))
                break;
            break;
        }
        case hash("lfo_delay"): {
            sfz::LFO::Control* ctl = getLfo(lfoIndex);
            if (!absl::SimpleAtof(opc.value, &ctl->delay))
                break;
            break;
        }
        case hash("lfo_fade"): {
            sfz::LFO::Control* ctl = getLfo(lfoIndex);
            if (!absl::SimpleAtof(opc.value, &ctl->fade))
                break;
            break;
        }
        case hash("lfo_count"): {
            sfz::LFO::Control* ctl = getLfo(lfoIndex);
            if (!absl::SimpleAtoi(opc.value, &ctl->countRepeats))
                break;
            break;
        }
        case hash("lfo_wave"): {
            sfz::LFO::Control::Sub* sub = getSub(lfoIndex, subwaveIndex);
            int wave;
            if (!sub || !absl::SimpleAtoi(opc.value, &wave))
                break;
            sub->wave = (sfz::LFO::Wave)wave;
            break;
        }
        case hash("lfo_offset"): {
            sfz::LFO::Control::Sub* sub = getSub(lfoIndex, subwaveIndex);
            if (!sub || !absl::SimpleAtof(opc.value, &sub->offset))
                break;
            break;
        }
        case hash("lfo_ratio"): {
            sfz::LFO::Control::Sub* sub = getSub(lfoIndex, subwaveIndex);
            if (!sub || !absl::SimpleAtof(opc.value, &sub->ratio))
                break;
            break;
        }
        case hash("lfo_scale"): {
            sfz::LFO::Control::Sub* sub = getSub(lfoIndex, subwaveIndex);
            if (!sub || !absl::SimpleAtof(opc.value, &sub->scale))
                break;
            break;
        }
        case hash("lfo_steps"): {
            sfz::LFO::Control::StepSequence* seq = getStepSeq(lfoIndex);
            if (!seq || !absl::SimpleAtoi(opc.value, &seq->numSteps))
                break;
            break;
        }
        case hash("lfo_step"): {
            float* step = getStep(lfoIndex, subwaveIndex);
            float value;
            if (!step || !absl::SimpleAtof(opc.value, &value))
                break;
            *step = value * 0.01f;
            break;
        }
        }
    }
}

/**
   Parser which gets the first region and extracts the configuration of LFO
 */
class LFOSetupParser : public sfz::Parser {
public:
    explicit LFOSetupParser(std::vector<sfz::LFO::Control>& ctl)
        : control(ctl)
    {
    }

    void callback(absl::string_view header, const std::vector<sfz::Opcode>& members) override
    {
        if (done || header != "region")
            return;

        done = true;

        configureLFOwithRegion(control, members);
    }

private:
    bool done = false;
    std::vector<sfz::LFO::Control>& control;
};

/**
   Program which loads LFO configuration and generates plot data for the given duration.
 */
int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 2) {
        usage();
        return 1;
    }

    fs::path sfzFilePath = argv[1];

    std::vector<sfz::LFO> lfos;
    std::vector<sfz::LFO::Control> ctls;

    LFOSetupParser parser(ctls);
    if (!parser.loadSfzFile(sfzFilePath)) {
        std::cerr << "Cannot load SFZ: " << sfzFilePath << "\n";
        return 1;
    }

    size_t numLfos = ctls.size();
    lfos.resize(numLfos);

    for (size_t l = 0; l < numLfos; ++l) {
        sfz::LFO& lfo = lfos[l];
        lfo.init(sampleRate);
        lfo.attachParameters(&ctls[l]);
    }

    size_t numFrames = (size_t)std::ceil(sampleRate * duration);
    std::unique_ptr<float[]> buffer { new float[numLfos * numFrames] };

    for (size_t l = 0; l < numLfos; ++l) {
        sfz::LFO& lfo = lfos[l];
        lfo.start();
        lfo.process(&buffer[l * numFrames], numFrames);
    }

    for (size_t i = 0; i < numFrames; ++i) {
        std::cout << (i / sampleRate);
        for (size_t l = 0; l < numLfos; ++l)
            std::cout << ' ' << buffer[i + l * numFrames];
        std::cout << '\n';
    }

    return 0;
}
