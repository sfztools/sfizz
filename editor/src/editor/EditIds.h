// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <cassert>

enum class EditId : int {
    SfzFile,
    Volume,
    Polyphony,
    Oversampling,
    PreloadSize,
    ScalaFile,
    ScalaRootKey,
    TuningFrequency,
    StretchTuning,
    UINumCurves,
    UINumMasters,
    UINumGroups,
    UINumRegions,
    UINumPreloadedSamples,
    UINumActiveVoices,
    UIActivePanel,
    ControllerChange,
};

struct EditRange {
    float def = 0.0;
    float min = 0.0;
    float max = 1.0;
    constexpr EditRange() = default;
    constexpr EditRange(float def, float min, float max)
        : def(def), min(min), max(max) {}
    static EditRange get(EditId id);
};
