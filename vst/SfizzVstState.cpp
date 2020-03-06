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

    if (!s.readInt32(oversampling))
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

    if (!s.writeInt32(oversampling))
        return kResultFalse;

    return kResultTrue;
}

tresult SfizzUiState::load(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);

    uint64 version = 0;
    if (!s.readInt64u(version))
        return kResultFalse;

    if (!s.readInt32u(activePanel))
        return kResultFalse;

    return kResultTrue;
}

tresult SfizzUiState::store(IBStream* state) const
{
    IBStreamer s(state, kLittleEndian);

    if (!s.writeInt64u(currentStateVersion))
        return kResultFalse;

    if (!s.writeInt32u(activePanel))
        return kResultFalse;

    return kResultTrue;
}

///
int SfizzMisc::adaptOversamplingFactor(int valueDenorm)
{
    if (valueDenorm >= 8)
        return SFIZZ_OVERSAMPLING_X8;
    else if (valueDenorm >= 4)
        return SFIZZ_OVERSAMPLING_X4;
    else if (valueDenorm >= 2)
        return SFIZZ_OVERSAMPLING_X2;
    else
        return SFIZZ_OVERSAMPLING_X1;
}
