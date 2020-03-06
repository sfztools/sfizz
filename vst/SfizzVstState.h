// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <string>

using namespace Steinberg;

// number of MIDI CC
enum {
    kNumControllerParams = 128,
};

// parameters
enum {
    kPidVolume,
    kPidNumVoices,
    kPidOversampling,
    kPidPreloadSize,
    kPidMidiAftertouch,
    kPidMidiPitchBend,
    kPidMidiCC0,
    kPidMidiCCLast = kPidMidiCC0 + kNumControllerParams - 1,
    /* Reserved */
};

class SfizzVstState {
public:
    std::string sfzFile;
    float volume = 0;
    int numVoices = 64;
    int oversamplingLog2 = 0;
    int preloadSize = 8192;

    static constexpr uint64 currentStateVersion = 0;

    tresult load(IBStream* state);
    tresult store(IBStream* state) const;
};

class SfizzUiState {
public:
    unsigned activePanel = 0;

    static constexpr uint64 currentStateVersion = 0;

    tresult load(IBStream* state);
    tresult store(IBStream* state) const;
};

struct SfizzParameterRange {
    float def = 0.0;
    float min = 0.0;
    float max = 1.0;

    constexpr SfizzParameterRange() {}
    constexpr SfizzParameterRange(float def, float min, float max) : def(def), min(min), max(max) {}

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
};

static constexpr SfizzParameterRange kParamVolumeRange(0.0, -60.0, +6.0);
static constexpr SfizzParameterRange kParamNumVoicesRange(64.0, 1.0, 256.0);
static constexpr SfizzParameterRange kParamOversamplingRange(0.0, 0.0, 3.0);
static constexpr SfizzParameterRange kParamPreloadSizeRange(8192.0, 1024.0, 65536.0);
