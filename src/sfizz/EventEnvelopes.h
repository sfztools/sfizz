// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "Config.h"
#include "LeakDetector.h"
#include <absl/types/span.h>
#include <functional>
#include <type_traits>
#include <vector>

namespace sfz {
/**
 * @brief Describes a simple envelope that can be polled in a blockwise
 * manner. It works by storing "events" in the immediate future and linearly
 * interpolating between these events. This envelope can also transform its
 * incoming target points through a lambda, although the lambda function is applied before the interpolation.
 *
 * The way to use this class is by repeatedly calling `registerEvent` and then
 * `getBlock` to get a block of interpolated values in between the specified events.
 * You should only register events whose timestamps are below the size of the block
 * you will require when calling `getBlock`.
 *
 * @tparam Type
 */
template <class Type>
class EventEnvelope {
public:
    /**
     * @brief Construct a new linear envelope with a default memory size for
     * incoming events.
     *
     */
    EventEnvelope();
    /**
     * @brief Construct a new linear envelope with a specific memory size for
     * incoming events as well as a transformation function for incoming events.
     *
     * @param maxCapacity
     * @param function
     */
    EventEnvelope(int maxCapacity, std::function<Type(Type)> function);
    /**
     * @brief Set the maximum memory size for incoming events
     *
     * @param maxCapacity
     */
    void setMaxCapacity(int maxCapacity);
    /**
     * @brief Set the transformation function for the value of incoming events.
     *
     * @param function
     */
    void setFunction(std::function<Type(Type)> function);
    /**
     * @brief Register a new event. Note that the timestamp of the new value should
     * be less than the future call to `getBlock` otherwise the event will be ignored.
     *
     * @param timestamp
     * @param inputValue
     */
    void registerEvent(int timestamp, Type inputValue);
    /**
     * @brief Clear all events in memory
     *
     */
    void clear();
    /**
     * @brief Reset the envelope and clears the memory.
     *
     * @param value
     */
    void reset(Type value = 0.0);
    /**
     * @brief Get a block of interpolated values between events previously registered
     * using `registerEvent`.
     *
     * @param output
     */
    virtual void getBlock(absl::Span<Type> output);
    /**
     * @brief  Get a block of interpolated values with a forced quantization. The
     * values within the block will vary in quantization steps.
     *
     * @param output
     * @param quantizationStep
     */
    virtual void getQuantizedBlock(absl::Span<Type> output, Type quantizationStep);
protected:
    std::vector<std::pair<int, Type>> events;
    Type currentValue { 0.0 };
private:
    static_assert(std::is_arithmetic<Type>::value, "Type should be arithmetic");
    std::function<Type(Type)> function { [](Type input) { return input; } };
    int maxCapacity { config::defaultSamplesPerBlock };
    void prepareEvents();
    bool resetEvents { false };
    LEAK_DETECTOR(EventEnvelope);
};


/**
 * @brief Describes a simple linear envelope.
 *
 * @tparam Type
 */
template <class Type>
class LinearEnvelope: public EventEnvelope<Type> {
public:
    void getBlock(absl::Span<Type> output) final;
    void getQuantizedBlock(absl::Span<Type> output, Type quantizationStep) final;
};
}
