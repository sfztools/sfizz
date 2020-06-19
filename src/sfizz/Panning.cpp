#include "Panning.h"
#include <array>
#include <cmath>

namespace sfz
{
// Number of elements in the table, odd for equal volume at center
constexpr int panSize = 4095;

// Table of pan values for the left channel, extra element for safety
static const auto panData = []()
{
    std::array<float, panSize + 1> pan;
    int i = 0;

    for (; i < panSize; ++i)
        pan[i] = std::cos(i * (piTwo<double>() / (panSize - 1)));

    for (; i < static_cast<int>(pan.size()); ++i)
        pan[i] = pan[panSize - 1];

    return pan;
}();

float panLookup(float pan)
{
    // reduce range, round to nearest
    int index = lroundPositive(pan * (panSize - 1));
    return panData[index];
}

void pan(const float* panEnvelope, float* leftBuffer, float* rightBuffer, unsigned size) noexcept
{
    const auto sentinel = panEnvelope + size;
    while (panEnvelope < sentinel) {
        auto p =(*panEnvelope + 1.0f) * 0.5f;
        p = clamp(p, 0.0f, 1.0f);
        *leftBuffer *= panLookup(p);
        *rightBuffer *= panLookup(1 - p);
        incrementAll(panEnvelope, leftBuffer, rightBuffer);
    }
}

void width(const float* widthEnvelope, float* leftBuffer, float* rightBuffer, unsigned size) noexcept
{
    const auto sentinel = widthEnvelope + size;
    while (widthEnvelope < sentinel) {
        float w = (*widthEnvelope + 1.0f) * 0.5f;
        w = clamp(w, 0.0f, 1.0f);
        const auto coeff1 = panLookup(w);
        const auto coeff2 = panLookup(1 - w);
        const auto l = *leftBuffer;
        const auto r = *rightBuffer;
        *leftBuffer = l * coeff2 + r * coeff1;
        *rightBuffer = l * coeff1 + r * coeff2;
        incrementAll(widthEnvelope, leftBuffer, rightBuffer);
    }
}
}
