// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstState.h"
#include <mutex>
#include <cstring>

tresult SfizzVstState::load(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);

    uint64 version = 0;
    if (!s.readInt64u(version))
        return kResultFalse;

    while (const char* key = s.readStr8()) {
        if (!std::strcmp(key, "SfzFile")) {
            const char* value = s.readStr8();
            if (!value)
                return kResultFalse;
            sfzFile = value;
        }
    }

    return kResultTrue;
}

tresult SfizzVstState::store(IBStream* state) const
{
    IBStreamer s(state, kLittleEndian);

    if (!s.writeInt64u(currentStateVersion))
        return kResultFalse;

    if (!s.writeStr8("SfzFile") || !s.writeStr8(sfzFile.c_str()))
        return kResultFalse;

    return kResultTrue;
}
