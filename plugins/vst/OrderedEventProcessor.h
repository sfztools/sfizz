// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <memory>

using namespace Steinberg;

template <class R>
class OrderedEventProcessor {
public:
    virtual ~OrderedEventProcessor() {}

    void initializeEventProcessor(const Vst::ProcessSetup& setup, int32 paramCount, int32 subdivSize = 128);
    void processUnorderedEvents(int32 numSamples, Vst::IParameterChanges* pcs, Vst::IEventList* evs);

private:
    int32 paramCount_ { 0 };
    int32 subdivSize_ { 0 };
    struct SubdivChange {
        SubdivChange(int32 offset, Vst::ParamID id, Vst::ParamValue value)
        : offset(offset), id(std::move(id)), value(std::move(value)) {}
        int32 offset;
        Vst::ParamID id {};
        Vst::ParamValue value {};
    };
    std::vector<SubdivChange> subdivChanges_;
    std::vector<int32> queuePositions_;
};

#include "OrderedEventProcessor.hpp"
