#pragma once
#include "Globals.h"
#include <tuple>
#include <array>

// enum class SVFReturn { BP_LP_HP, BP_LP, BP};

// template<SVFReturn ReturnType = SVFReturn::BP>
class StateVariableFilter
{
public:
    StateVariableFilter() = default;
    StateVariableFilter(float R, float gain)
    : R(R), gain(gain)
    {

    }

    // Only use specializations
    template<class InputType, class... OutputTypes>
    auto process(const InputType& input, OutputTypes... outputs) = delete;
    
private:
    std::array<float, 2> state;
    float R { 0.0 };
    float gain { 1.0 };
};