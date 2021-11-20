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
    subdivChanges_.reserve(subdivSize * paramCount);
    queuePositions_.reserve(paramCount);
}

template <class R>
void OrderedEventProcessor<R>::processUnorderedEvents(int32 numSamples, Vst::IParameterChanges* pcs, Vst::IEventList* evs)
{
    if (!pcs || !evs)
        return;

    R& receiver = *static_cast<R*>(this);
    int32 sampleIndex = 0;
    int32 subdivNumber = 0;
    const int32 subdivSize = subdivSize_;
    if (subdivSize == 0)
        return;

    int32 eventIdx = 0;
    int32 eventCount = evs->getEventCount();
    Vst::Event event;
    bool hasEvent = false;
    if (eventCount > 0) {
        evs->getEvent(eventIdx++, event);
        hasEvent = true;
    }

    // Assume that there would be as many parameter changes as there are parameters, but some hosts are nice
    // and send multiple queues per parameter. Clamp the number of considered parameter queues to the number of parameters.
    queuePositions_.clear();
    const int32 parameterCount = pcs->getParameterCount();
    const int32 queueCapacity = static_cast<int32>(queuePositions_.capacity());
    assert(queueCapacity >= parameterCount);
    const int32 consideredQueueCount = std::min(queueCapacity, parameterCount);
    queuePositions_.resize(consideredQueueCount, 0);

    while (sampleIndex < numSamples || subdivNumber == 0) {
        const int32 subdivCurrentSize = std::min(numSamples - sampleIndex, subdivSize);
        const int32 lastOffset = sampleIndex + subdivSize;

        // Queue all changes for the subdiv
        subdivChanges_.clear();
        for (int32 qIdx = 0; qIdx < consideredQueueCount; ++qIdx) {
            auto* vq = pcs->getParameterData(qIdx);
            if (!vq)
                continue;

            int32 offset;
            Vst::ParamID id = vq->getParameterId();
            Vst::ParamValue value;

            int32& queuePosition = queuePositions_[qIdx];
            while (queuePosition < vq->getPointCount()) {
                vq->getPoint(queuePosition, offset, value);
                if (offset > lastOffset)
                    break;

                subdivChanges_.emplace_back(offset, id, value);
                queuePosition++;
            }
        }

        // Sort the changes
        std::sort(subdivChanges_.begin(), subdivChanges_.end(), [] (const SubdivChange& lhs, const SubdivChange& rhs) {
            return lhs.offset < rhs.offset;
        });

        // Play the parameter changes, interleaving events as needed in between
        for (const auto& change: subdivChanges_) {
            while (hasEvent && event.sampleOffset < change.offset) {
                receiver.playOrderedEvent(event);
                hasEvent = (eventIdx < eventCount);
                evs->getEvent(eventIdx++, event);
            }
            receiver.playOrderedParameter(change.offset, change.id, change.value);
        }

        sampleIndex += subdivCurrentSize;
        ++subdivNumber;
    }

    // Play the remaining events
    while (hasEvent) {
        receiver.playOrderedEvent(event);
        hasEvent = (eventIdx < eventCount);
        evs->getEvent(eventIdx++, event);
    }
}
