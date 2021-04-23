// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "OrderedEventProcessor.h"
#include <algorithm>
#include <cstdio>
#include <cassert>

template <class R>
void OrderedEventProcessor<R>::initializeEventProcessor(const Vst::ProcessSetup& setup, int32 paramCount, int32 subdivSize)
{
    paramCount_ = paramCount;
    subdivSize_ = subdivSize;
    queues_.reset(new Cell<QueueStatus>[paramCount]);
    pointsBySample_.reset(new ParameterAndValue[paramCount * subdivSize]);
    numPointsBySample_.reset(new uint32[subdivSize]);
}

template <class R>
void OrderedEventProcessor<R>::processUnorderedEvents(int32 numSamples, Vst::IParameterChanges* pcs, Vst::IEventList* evs)
{
    int32 sampleIndex = 0;
    int32 subdivNumber = 0;
    const int32 subdivSize = subdivSize_;

    startProcessing(evs, pcs);

    while (sampleIndex < numSamples || subdivNumber == 0) {
        int32 subdivCurrentSize = std::min(numSamples - sampleIndex, subdivSize);
        processSubdiv(evs, sampleIndex, sampleIndex + subdivCurrentSize - 1);
        sampleIndex += subdivCurrentSize;
        ++subdivNumber;
    }

    playRemainder(std::max(int32(0), numSamples - 1), evs);
}

template <class R>
void OrderedEventProcessor<R>::startProcessing(Vst::IEventList* evs, Vst::IParameterChanges* pcs)
{
    // collect the parameter queues which have values on them
    // push them onto a work list
    Cell<QueueStatus>* head = nullptr;
    Cell<QueueStatus>* queues = queues_.get();

    if (pcs) {
        const int32 count = pcs->getParameterCount();
        for (int32 index = count; index-- > 0; ) {
            Vst::IParamValueQueue* vq = pcs->getParameterData(index);
            if (vq) {
                // Note: expectation that hosts does not send more than one
                //       queue for one parameter
                Vst::ParamID id = vq->getParameterId();
                Cell<QueueStatus>* cell = &queues[id];
                cell->next = head;
                cell->value.queue = vq;
                cell->value.pointIndex = 0;
                cell->value.pointCount = vq->getPointCount();
                head = cell;
            }
        }
    }

    queueList_ = head;

    // position ourselves to the start of the event list
    eventIndex_ = 0;
    eventCount_ = evs ? evs->getEventCount() : 0;
    haveCurrentEvent_ = eventCount_ > 0 &&
        evs->getEvent(eventIndex_, currentEvent_) == kResultTrue;
}

template <class R>
void OrderedEventProcessor<R>::processSubdiv(Vst::IEventList* evs, int32 firstOffset, int32 lastOffset)
{
    sortSubdiv(firstOffset, lastOffset);
    playSubdiv(evs, firstOffset, lastOffset);
}

template <class R>
void OrderedEventProcessor<R>::sortSubdiv(int32 firstOffset, int32 lastOffset)
{
    // this collects parameter changes from the subdivision that goes from
    // first offset to last offset, included.

    // these parameter changes are on a array of lists.
    // this 2D structure is backed by a contiguous array dimensioned for
    // the worst case.

    // Example:
    //     pointsBySample (column-major →)   |  numPointsBySample[sample]
    //  ====================================================================
    //     sample 0   [P1] [P2] [  ] [  ]    |  2
    //     sample 1   [P3] [  ] [  ] [  ]    |  1
    //     sample 2   [  ] [  ] [  ] [  ]    |  0
    //     sample 3   [P1] [P2] [P3] [P4]    |  4

    Cell<QueueStatus>* head = queueList_;
    Cell<QueueStatus>* queues = queues_.get();
    const int32 paramCount = paramCount_;
    ParameterAndValue* pointsBySample = pointsBySample_.get();
    uint32* numPointsBySample = numPointsBySample_.get();

    std::fill_n(numPointsBySample, lastOffset - firstOffset + 1, 0);

    Cell<QueueStatus>* cur = head;
    Cell<QueueStatus>* prev = nullptr;

    while (cur) {
        Vst::IParamValueQueue* vq = cur->value.queue;
        const Vst::ParamID id = Vst::ParamID(std::distance(queues, cur));

        int32 pointIndex = cur->value.pointIndex;
        int32 pointCount = cur->value.pointCount;

        int32 previousOffset = firstOffset;
        while (pointIndex < pointCount) {
            int32 sampleOffset;
            Vst::ParamValue value;

            if (vq->getPoint(pointIndex, sampleOffset, value) != kResultTrue) {
                ++pointIndex;
                continue;
            }

            if (sampleOffset > lastOffset)
                break;

            // ensure that offsets never go back
            if (sampleOffset < previousOffset)
                sampleOffset = previousOffset;

            int32 listSize = numPointsBySample[sampleOffset - firstOffset];
            ParameterAndValue* listItems = &pointsBySample[paramCount * (sampleOffset - firstOffset)];

            bool isDuplicatePoint = listSize > 0 && listItems[listSize - 1].id == id;
            if (isDuplicatePoint) {
                // protect against duplicates, which might overflow the list
                listItems[listSize - 1].value = value;
            }
            else {
                assert(listSize < paramCount);
                listItems[listSize].id = id;
                listItems[listSize].value = value;
                numPointsBySample[sampleOffset - firstOffset] = ++listSize;
            }

            previousOffset = sampleOffset;
            ++pointIndex;
        }

        if (pointIndex < pointCount) {
            cur->value.pointIndex = pointIndex;
            prev = cur;
        }
        else {
            // if no more points, take this queue off the list
            if (prev)
                prev->next = cur->next;
            else {
                head = cur->next;
                prev = nullptr;
            }
        }
        cur = cur->next;
    }

    queueList_ = head;
}

template <class R>
void OrderedEventProcessor<R>::playSubdiv(Vst::IEventList* evs, int32 firstOffset, int32 lastOffset)
{
    // go over the points in sample order, and play them intermittently with the
    // event queue according to the sample offsets.

    R& receiver = *static_cast<R*>(this);

    const int32 paramCount = paramCount_;
    const ParameterAndValue* pointsBySample = pointsBySample_.get();
    const uint32* numPointsBySample = numPointsBySample_.get();

    for (int32 sampleOffset = firstOffset; sampleOffset <= lastOffset; ++sampleOffset) {
        const int32 listSize = numPointsBySample[sampleOffset - firstOffset];
        const ParameterAndValue* listItems = &pointsBySample[paramCount * (sampleOffset - firstOffset)];
        playEventsUpTo(evs, sampleOffset);
        for (int32 i = 0; i < listSize; ++i) {
            const ParameterAndValue item = listItems[i];
            receiver.playOrderedParameter(sampleOffset, item.id, item.value);
        }
    }
}

template <class R>
void OrderedEventProcessor<R>::playRemainder(int32 sampleOffset, Vst::IEventList* evs)
{
    // play any remaining events and parameters in arbitrary order,
    // disregarding their respective offsets

    R& receiver = *static_cast<R*>(this);

    int32 eventIndex = eventIndex_;
    int32 eventCount = eventCount_;
    bool haveCurrentEvent = haveCurrentEvent_;
    while (haveCurrentEvent) {
        currentEvent_.sampleOffset = std::min(sampleOffset, currentEvent_.sampleOffset);
        receiver.playOrderedEvent(currentEvent_);
        haveCurrentEvent = false;
        while (!haveCurrentEvent && ++eventIndex < eventCount)
            haveCurrentEvent = evs->getEvent(eventIndex, currentEvent_) == kResultTrue;
    }

    Cell<QueueStatus>* queues = queues_.get();
    for (Cell<QueueStatus>* cur = queueList_; cur; cur = cur->next) {
        for (int32 i = cur->value.pointIndex, n = cur->value.pointCount; i < n; ++i) {
            Vst::IParamValueQueue* vq = cur->value.queue;
            const Vst::ParamID id = Vst::ParamID(std::distance(queues, cur));
            int32 unusedSampleOffset;
            Vst::ParamValue value;
            if (vq->getPoint(i, unusedSampleOffset, value) == kResultTrue)
                receiver.playOrderedParameter(sampleOffset, id, value);
        }
    }
}

template <class R>
void OrderedEventProcessor<R>::playEventsUpTo(Vst::IEventList* evs, int32 sampleOffset)
{
    R& receiver = *static_cast<R*>(this);

    int32 index = eventIndex_;
    int32 count = eventCount_;
    bool haveCurrentEvent = haveCurrentEvent_;

    while (haveCurrentEvent && currentEvent_.sampleOffset <= sampleOffset) {
        receiver.playOrderedEvent(currentEvent_);
        haveCurrentEvent = false;
        while (!haveCurrentEvent && ++index < count)
            haveCurrentEvent = evs->getEvent(index, currentEvent_) == kResultTrue;
    }

    eventIndex_ = index;
    haveCurrentEvent_ = haveCurrentEvent;
}
