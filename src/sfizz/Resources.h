// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "FilePool.h"
#include "FilterPool.h"
#include "EQPool.h"
#include "Logger.h"

namespace sfz
{
class WavetableMulti;

struct Resources
{
    MidiState midiState;
    Logger logger;
    FilePool filePool { logger };
    FilterPool filterPool { midiState };
    EQPool eqPool { midiState };

    const WavetableMulti* waveSin { nullptr };
    const WavetableMulti* waveTriangle { nullptr };
    const WavetableMulti* waveSaw { nullptr };
    const WavetableMulti* waveSquare { nullptr };

    Resources();
};
}
