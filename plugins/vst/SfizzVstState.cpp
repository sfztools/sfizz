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

    if (version > currentStateVersion)
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

    if (version >= 3) {
        if (!s.readInt32(sampleQuality))
            return kResultFalse;

        if (!s.readInt32(oscillatorQuality))
            return kResultFalse;
    }
    else {
        sampleQuality = defaults.sampleQuality;
        oscillatorQuality = defaults.oscillatorQuality;
    }

    controllers.clear();
    if (version >= 2) {
        uint32 count;
        if (!s.readInt32u(count))
            return kResultFalse;
        controllers.resize(0x10000);
        uint32 size = 0;
        for (uint32 i = 0; i < count; ++i) {
            uint16 cc;
            float value;
            if (!s.readInt16u(cc) || !s.readFloat(value))
                return kResultFalse;
            controllers[cc] = value;
            size = std::max(size, uint32(cc) + 1);
        }
        controllers.resize(size);
        controllers.shrink_to_fit();
    }

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

    if (!s.writeInt32(sampleQuality))
        return kResultFalse;

    if (!s.writeInt32(oscillatorQuality))
        return kResultFalse;

    {
        uint32 ccCount = 0;
        uint32 ccLimit = uint32(std::min(controllers.size(), size_t(0x10000)));
        for (uint32_t cc = 0; cc < ccLimit; ++cc)
            ccCount += controllers[cc] != absl::nullopt;
        if (!s.writeInt32u(ccCount))
            return kResultFalse;
        for (uint32_t cc = 0; cc < ccLimit; ++cc) {
            if (absl::optional<float> ccValue = controllers[cc]) {
                if (!s.writeInt16u(uint16(cc)) || !s.writeFloat(*ccValue))
                    return kResultFalse;
            }
        }
    }

    return kResultTrue;
}

constexpr uint64 SfizzVstState::currentStateVersion;
