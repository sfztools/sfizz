// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz/Config.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <stdexcept>

using namespace Steinberg;

// parameters
enum {
    kPidVolume,
    kPidNumVoices,
    kPidOversampling,
    kPidPreloadSize,
    kPidScalaRootKey,
    kPidTuningFrequency,
    kPidStretchedTuning,
    kPidSampleQuality,
    kPidOscillatorQuality,
    kPidAftertouch,
    kPidPitchBend,
    kPidCC0,
    kPidCCLast = kPidCC0 + sfz::config::numCCs - 1,
    /* Reserved */
    kNumParameters,
};

struct SfizzRange {
    float def = 0.0;
    float min = 0.0;
    float max = 1.0;

    constexpr SfizzRange() {}
    constexpr SfizzRange(float def, float min, float max) : def(def), min(min), max(max) {}

    constexpr float normalize(float x) const noexcept
    {
        return (x - min) / (max - min);
    }

    constexpr float denormalize(float x) const noexcept
    {
        return min + x * (max - min);
    }

    Vst::RangeParameter* createParameter(const Vst::TChar *title, Vst::ParamID tag, const Vst::TChar *units = nullptr, int32 stepCount = 0, int32 flags = Vst::ParameterInfo::kCanAutomate, Vst::UnitID unitID = Vst::kRootUnitId, const Vst::TChar *shortTitle = nullptr) const
    {
        return new Vst::RangeParameter(title, tag, units, min, max, def, stepCount, flags, unitID, shortTitle);
    }

    static SfizzRange getForParameter(Vst::ParamID id)
    {
        switch (id) {
        case kPidVolume:
            return {0.0, -60.0, +6.0};
        case kPidNumVoices:
            return {64.0, 1.0, 256.0};
        case kPidOversampling:
            return {0.0, 0.0, 3.0};
        case kPidPreloadSize:
            return {8192.0, 1024.0, 65536.0};
        case kPidScalaRootKey:
            return {60.0, 0.0, 127.0};
        case kPidTuningFrequency:
            return {440.0, 300.0, 500.0};
        case kPidStretchedTuning:
            return {0.0, 0.0, 1.0};
        case kPidSampleQuality:
            return {2.0, 0.0, 10.0};
        case kPidOscillatorQuality:
            return {1.0, 0.0, 3.0};
        case kPidAftertouch:
            return {0.0, 0.0, 1.0};
        case kPidPitchBend:
            return {0.0, -1.0, 1.0};
        default:
            if (id >= kPidCC0 && id <= kPidCCLast)
                return {0.0, 0.0, 1.0};
            throw std::runtime_error("Bad parameter ID");
        }
    }
};

inline int32 integerLog2(int32 x)
{
    int32 l = 0;
    for (; x > 1; x /= 2) ++l;
    return l;
}
