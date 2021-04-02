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
    FallbackFilesDir,
    //
    Key0,
    KeyLast = Key0 + 128 - 1,
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

#define DEFINE_EDIT_ID_RANGE_HELPERS(type, Type, IdPrefix)          \
    inline EditId editIdFor##Type(int value)                        \
    {                                                               \
        return EditId(int(EditId::IdPrefix##0) + value);            \
    }                                                               \
    inline int type##ForEditId(EditId id)                           \
    {                                                               \
        return int(id) - int(EditId::IdPrefix##0);                  \
    }                                                               \
    inline bool editIdIs##Type(EditId id)                           \
    {                                                               \
        return int(id) >= int(EditId::IdPrefix##0) &&               \
            int(id) <= int(EditId::IdPrefix##Last);                 \
    }

// defines editIdForCC, ccForEditId, editIdIsCC, etc..
DEFINE_EDIT_ID_RANGE_HELPERS(cc, CC, Controller)
DEFINE_EDIT_ID_RANGE_HELPERS(key, Key, Key)

#undef DEFINE_EDIT_ID_RANGE_HELPERS
