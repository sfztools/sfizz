// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz/Config.h"
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
    CanEditUserFilesDir,
    UserFilesDir,
    //
    Controller0,
    ControllerLast = Controller0 + sfz::config::numCCs - 1,
    //
    UINumCurves,
    UINumMasters,
    UINumGroups,
    UINumRegions,
    UINumPreloadedSamples,
    UINumActiveVoices,
    UIActivePanel,
};

struct EditRange {
    float def = 0.0;
    float min = 0.0;
    float max = 1.0;
    constexpr EditRange() = default;
    constexpr EditRange(float def, float min, float max)
        : def(def), min(min), max(max) {}
    float extent() const noexcept { return max - min; }
    static EditRange get(EditId id);
};

inline EditId editIdForCC(int cc)
{
    return EditId(int(EditId::Controller0) + cc);
}
inline int ccForEditId(EditId id)
{
    return int(id) - int(EditId::Controller0);
}
inline bool editIdIsCC(EditId id)
{
    return int(id) >= int(EditId::Controller0) &&
        int(id) <= int(EditId::ControllerLast);
}
