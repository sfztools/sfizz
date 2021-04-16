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
    SampleQuality,
    OscillatorQuality,
    CanEditUserFilesDir,
    UserFilesDir,
    FallbackFilesDir,
    //
    #define KEY_RANGE(Name) Name##0, Name##Last = Name##0 + 128 - 1
    #define CC_RANGE(Name) Name##0, Name##Last = Name##0 + sfz::config::numCCs - 1
    //
    KEY_RANGE(Key),
    CC_RANGE(Controller),
    //
    KEY_RANGE(KeyUsed),
    KEY_RANGE(KeyLabel),
    KEY_RANGE(KeyswitchUsed),
    KEY_RANGE(KeyswitchLabel),
    CC_RANGE(ControllerUsed),
    CC_RANGE(ControllerDefault),
    CC_RANGE(ControllerLabel),
    //
    UINumCurves,
    UINumMasters,
    UINumGroups,
    UINumRegions,
    UINumPreloadedSamples,
    UINumActiveVoices,
    UIActivePanel,
    //
    BackgroundImage,
    //
    PluginFormat,
    PluginHost,
    //
    #undef KEY_RANGE
    #undef CC_RANGE
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
    inline bool editIdIs##Type(EditId id)                           \
    {                                                               \
        return int(id) >= int(EditId::IdPrefix##0) &&               \
            int(id) <= int(EditId::IdPrefix##Last);                 \
    }                                                               \
    inline EditId editIdFor##Type(int value)                        \
    {                                                               \
        EditId id = EditId(int(EditId::IdPrefix##0) + value);       \
        assert(editIdIs##Type(id));                                 \
        return id;                                                  \
    }                                                               \
    inline int type##ForEditId(EditId id)                           \
    {                                                               \
        assert(editIdIs##Type(id));                                 \
        return int(id) - int(EditId::IdPrefix##0);                  \
    }

// defines editIdForCC, ccForEditId, editIdIsCC, etc..
DEFINE_EDIT_ID_RANGE_HELPERS(cc, CC, Controller)
DEFINE_EDIT_ID_RANGE_HELPERS(key, Key, Key)
DEFINE_EDIT_ID_RANGE_HELPERS(keyUsed, KeyUsed, KeyUsed)
DEFINE_EDIT_ID_RANGE_HELPERS(keyLabel, KeyLabel, KeyLabel)
DEFINE_EDIT_ID_RANGE_HELPERS(keyswitchUsed, KeyswitchUsed, KeyswitchUsed)
DEFINE_EDIT_ID_RANGE_HELPERS(keyswitchLabel, KeyswitchLabel, KeyswitchLabel)
DEFINE_EDIT_ID_RANGE_HELPERS(ccUsed, CCUsed, ControllerUsed)
DEFINE_EDIT_ID_RANGE_HELPERS(ccDefault, CCDefault, ControllerDefault)
DEFINE_EDIT_ID_RANGE_HELPERS(ccLabel, CCLabel, ControllerLabel)

#undef DEFINE_EDIT_ID_RANGE_HELPERS
