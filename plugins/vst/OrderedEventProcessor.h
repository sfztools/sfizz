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
    void startProcessing(Vst::IEventList* evs, Vst::IParameterChanges* pcs);
    void processSubdiv(Vst::IEventList* evs, int32 firstOffset, int32 lastOffset);

    void sortSubdiv(int32 firstOffset, int32 lastOffset);
    void playSubdiv(Vst::IEventList* evs, int32 firstOffset, int32 lastOffset);

    void playRemainder(int32 sampleOffset, Vst::IEventList* evs);

    void playEventsUpTo(Vst::IEventList* evs, int32 sampleOffset);

private:
    template <class T> struct Cell {
        Cell<T>* next = nullptr;
        T value {};
    };

    struct QueueStatus {
        Vst::IParamValueQueue* queue = nullptr;
        int32 pointIndex = 0;
        int32 pointCount = 0;
    };

    int32 paramCount_ = 0;
    int32 subdivSize_ = 0;
    std::unique_ptr<Cell<QueueStatus>[]> queues_;
    Cell<QueueStatus>* queueList_ = nullptr;

    struct ParameterAndValue {
        Vst::ParamID id {};
        Vst::ParamValue value {};
    };

    std::unique_ptr<ParameterAndValue[]> pointsBySample_;
    std::unique_ptr<uint32[]> numPointsBySample_;

    int32 eventIndex_ = 0;
    int32 eventCount_ = 0;
    bool haveCurrentEvent_ = false;
    Vst::Event currentEvent_ {};
};

#include "OrderedEventProcessor.hpp"
