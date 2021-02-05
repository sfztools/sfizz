// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstState.h"
#include <sfizz.h>
#include <mutex>
#include <cstring>

tresult SfizzVstState::load(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);

    uint64 version = 0;
    if (!s.readInt64u(version))
        return kResultFalse;

    if (const char* str = s.readStr8())
        sfzFile = str;
    else
        return kResultFalse;

    if (!s.readFloat(volume))
        return kResultFalse;

    if (!s.readInt32(numVoices))
        return kResultFalse;

    if (!s.readInt32(oversamplingLog2))
        return kResultFalse;

    if (!s.readInt32(preloadSize))
        return kResultFalse;

    const SfizzVstState defaults;

    if (version >= 1) {
        if (const char* str = s.readStr8())
            scalaFile = str;
        else
            return kResultFalse;

        if (!s.readInt32(scalaRootKey))
            return kResultFalse;

        if (!s.readFloat(tuningFrequency))
            return kResultFalse;

        if (!s.readFloat(stretchedTuning))
            return kResultFalse;
    }
    else {
        scalaFile = defaults.scalaFile;
        scalaRootKey = defaults.scalaRootKey;
        tuningFrequency = defaults.tuningFrequency;
        stretchedTuning = defaults.stretchedTuning;
    }

    if (version > 1)
        return kResultFalse;

    return kResultTrue;
}

tresult SfizzVstState::store(IBStream* state) const
{
    IBStreamer s(state, kLittleEndian);

    if (!s.writeInt64u(currentStateVersion))
        return kResultFalse;

    if (!s.writeStr8(sfzFile.c_str()))
        return kResultFalse;

    if (!s.writeFloat(volume))
        return kResultFalse;

    if (!s.writeInt32(numVoices))
        return kResultFalse;

    if (!s.writeInt32(oversamplingLog2))
        return kResultFalse;

    if (!s.writeInt32(preloadSize))
        return kResultFalse;

    if (!s.writeStr8(scalaFile.c_str()))
        return kResultFalse;

    if (!s.writeInt32(scalaRootKey))
        return kResultFalse;

    if (!s.writeFloat(tuningFrequency))
        return kResultFalse;

    if (!s.writeFloat(stretchedTuning))
        return kResultFalse;

    return kResultTrue;
}

constexpr uint64 SfizzVstState::currentStateVersion;
