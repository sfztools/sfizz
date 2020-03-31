#pragma once

#include "Config.h"
#include "Defaults.h"
#include "MathHelpers.h"
#include "SfzHelpers.h"
#include "OnePoleFilter.h"
#include "SIMDHelpers.h"
#include <array>

namespace sfz {
class Smoother {
public:
    Smoother();
    void setSmoothing(uint8_t smoothValue, float sampleRate);
    void reset(float value = 0.0f);
    void process(absl::Span<const float> input, absl::Span<float> output);

private:
    bool smoothing { false };
    OnePoleFilter<float> filter {};
};

}
