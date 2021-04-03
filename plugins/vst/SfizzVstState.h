// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "base/source/fstreamer.h"
#include <string>

using namespace Steinberg;

class SfizzVstState {
public:
    SfizzVstState() { sfzFile.reserve(8192); scalaFile.reserve(8192); }

    std::string sfzFile;
    float volume = 0;
    int32 numVoices = 64;
    int32 oversamplingLog2 = 0;
    int32 preloadSize = 8192;
    std::string scalaFile;
    int32 scalaRootKey = 60;
    float tuningFrequency = 440.0;
    float stretchedTuning = 0.0;

    static constexpr uint64 currentStateVersion = 1;

    tresult load(IBStream* state);
    tresult store(IBStream* state) const;
};

struct SfizzPlayState {
    uint32 activeVoices;
};
