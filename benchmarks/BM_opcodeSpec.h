#pragma once

#include "Range.h"

template<class T>
struct OpcodeSpec
{
    T defaultValue;
    sfz::Range<T> bounds;
    int flags { 0 };
};

constexpr OpcodeSpec<float> constexprSpec { 0.0f, sfz::Range<float>(0.0f, 0.5f), 1 << 2 };
extern const OpcodeSpec<float> constSpec;
