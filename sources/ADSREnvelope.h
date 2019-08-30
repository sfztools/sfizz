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
#include "LeakDetector.h"
#include <absl/types/span.h>
namespace sfz {

template <class Type>
class ADSREnvelope {
public:
    ADSREnvelope() = default;
    void reset(int attack, int release, Type sustain = 1.0, int delay = 0, int decay = 0, int hold = 0, Type start = 0.0, Type depth = 1) noexcept;
    Type getNextValue() noexcept;
    void getBlock(absl::Span<Type> output) noexcept;
    void startRelease(int releaseDelay) noexcept;
    bool isSmoothing() noexcept;

private:
    enum class State {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Done
    };
    State currentState { State::Done };
    Type currentValue { 0.0 };
    Type step { 0.0 };
    int delay { 0 };
    int attack { 0 };
    int decay { 0 };
    int release { 0 };
    int hold { 0 };
    Type start { 0 };
    Type peak { 0 };
    Type sustain { 0 };
    int releaseDelay { 0 };
    bool shouldRelease { false };
    LEAK_DETECTOR(ADSREnvelope);
};

}