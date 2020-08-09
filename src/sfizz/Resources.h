// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SynthConfig.h"
#include "FilePool.h"
#include "BufferPool.h"
#include "FilterPool.h"
#include "EQPool.h"
#include "Logger.h"
#include "Wavetables.h"
#include "Curve.h"
#include "Tuning.h"
#include "modulations/ModMatrix.h"
#include "absl/types/optional.h"

namespace sfz
{
class WavetableMulti;

struct Resources
{
    SynthConfig synthConfig;
    BufferPool bufferPool;
    MidiState midiState;
    Logger logger;
    CurveSet curves;
    FilePool filePool { logger };
    FilterPool filterPool { midiState };
    EQPool eqPool { midiState };
    WavetablePool wavePool;
    Tuning tuning;
    absl::optional<StretchTuning> stretch;
    ModMatrix modMatrix;

    void setSampleRate(float samplerate)
    {
        midiState.setSampleRate(samplerate);
        filterPool.setSampleRate(samplerate);
        eqPool.setSampleRate(samplerate);
        modMatrix.setSampleRate(samplerate);
    }

    void setSamplesPerBlock(int samplesPerBlock)
    {
        bufferPool.setBufferSize(samplesPerBlock);
        midiState.setSamplesPerBlock(samplesPerBlock);
        modMatrix.setSamplesPerBlock(samplesPerBlock);
    }

    void clear()
    {
        curves = CurveSet::createPredefined();
        filePool.clear();
        wavePool.clearFileWaves();
        logger.clear();
        midiState.reset();
        modMatrix.clear();
    }
};
}
