#include "Smoothers.h"

namespace sfz {

Smoother::Smoother()
{
}

void Smoother::setSmoothing(uint8_t smoothValue, float sampleRate)
{
    smoothing = (smoothValue > 0);
    if (smoothing) {
        filter.setGain(std::tan(1.0f / (2 * Default::smoothTauPerStep * smoothValue * sampleRate)));
    }
}

void Smoother::reset(float value)
{
    filter.reset(value);
}

void Smoother::process(absl::Span<const float> input, absl::Span<float> output)
{
    if (smoothing)
        filter.processLowpass(input, output);
    else
        copy<float>(input, output);
}

}
