// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Resources.h"
#include "Wavetables.h"

namespace sfz
{

static constexpr unsigned kTableSize = 1024;

// amplitude values are matched to reference
static constexpr double kAmplitudeSine = 0.625;
static constexpr double kAmplitudeTriangle = 0.625;
static constexpr double kAmplitudeSaw = 0.515;
static constexpr double kAmplitudeSquare = 0.515;

static const WavetableMulti* getWaveSin()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
        HarmonicProfile::getSine(), kAmplitudeSine, kTableSize);
    return &wave;
}

static const WavetableMulti* getWaveTriangle()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
        HarmonicProfile::getTriangle(), kAmplitudeTriangle, kTableSize);
    return &wave;
}

static const WavetableMulti* getWaveSaw()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
        HarmonicProfile::getSaw(), kAmplitudeSaw, kTableSize);
    return &wave;
}

static const WavetableMulti* getWaveSquare()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
        HarmonicProfile::getSquare(), kAmplitudeSquare, kTableSize);
    return &wave;
}

Resources::Resources()
    : waveSin(getWaveSin()),
      waveTriangle(getWaveTriangle()),
      waveSaw(getWaveSaw()),
      waveSquare(getWaveSquare())
{
}


}
