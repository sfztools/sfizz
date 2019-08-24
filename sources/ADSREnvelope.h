#pragma once
#include <absl/types/span.h>
#include "Helpers.h"
namespace sfz
{

template<class Type>
class ADSREnvelope
{
public:
    ADSREnvelope() = default;
    void reset(int attack, int release, Type sustain=1.0, int delay = 0, int decay = 0, int hold = 0, Type start=0.0, Type depth=1) noexcept;
    Type getNextValue() noexcept;
    void getBlock(absl::Span<Type> output) noexcept;
    void startRelease(int releaseDelay) noexcept;
    bool isSmoothing() noexcept;
private:
    enum class State
    {
        Delay, Attack, Hold, Decay, Sustain, Release, Done
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