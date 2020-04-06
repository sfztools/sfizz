#pragma once

#include "Range.h"
#include "Defaults.h"
#include "SfzHelpers.h"
#include "Resources.h"
#include "absl/types/span.h"

namespace sfz {

/**
 * @brief Compute a crossfade in value with respect to a crossfade range (note, velocity, cc, ...)
 */
template <class T, class U>
float crossfadeIn(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value < crossfadeRange.getStart())
        return 0.0f;

    const auto length = static_cast<float>(crossfadeRange.length());
    if (length == 0.0f)
        return 1.0f;

    else if (value < crossfadeRange.getEnd()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / length;
        if (curve == SfzCrossfadeCurve::power)
            return sqrt(crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return crossfadePosition;
    }

    return 1.0f;
}

/**
 * @brief Compute a crossfade out value with respect to a crossfade range (note, velocity, cc, ...)
 */
template <class T, class U>
float crossfadeOut(const sfz::Range<T>& crossfadeRange, U value, SfzCrossfadeCurve curve)
{
    if (value > crossfadeRange.getEnd())
        return 0.0f;

    const auto length = static_cast<float>(crossfadeRange.length());
    if (length == 0.0f)
        return 1.0f;

    else if (value > crossfadeRange.getStart()) {
        const auto crossfadePosition = static_cast<float>(value - crossfadeRange.getStart()) / length;
        if (curve == SfzCrossfadeCurve::power)
            return std::sqrt(1 - crossfadePosition);
        if (curve == SfzCrossfadeCurve::gain)
            return 1 - crossfadePosition;
    }

    return 1.0f;
}

template <class F>
void linearEnvelope(const EventVector& events, absl::Span<float> envelope, F&& lambda)
{
    ASSERT(events.size() > 0);
    ASSERT(events[0].delay == 0);
    if (envelope.size() == 0)
        return;

    const auto maxDelay = static_cast<int>(envelope.size() - 1);

    auto lastValue = lambda(events[0].value);
    auto lastDelay = events[0].delay;
    for (unsigned i = 1; i < events.size() && lastDelay < maxDelay; ++i) {
        const auto length = min(events[i].delay, maxDelay) - lastDelay;
        const auto step = (lambda(events[i].value) - lastValue) / length;
        lastValue = linearRamp<float>(envelope.subspan(lastDelay, length), lastValue, step);
        lastDelay += length;
    }
    fill<float>(envelope.subspan(lastDelay), lastValue);
}

template <class F>
void linearEnvelope(const EventVector& events, absl::Span<float> envelope, F&& lambda, float step)
{
    ASSERT(events.size() > 0);
    ASSERT(events[0].delay == 0);
    ASSERT(step != 0.0);

    if (envelope.size() == 0)
        return;

    auto quantize = [step](float value) -> float {
        return std::floor(value / step) * step;
    };
    const auto maxDelay = static_cast<int>(envelope.size() - 1);

    auto lastValue = quantize(lambda(events[0].value));
    auto lastDelay = events[0].delay;
    for (unsigned i = 1; i < events.size() && lastDelay < maxDelay; ++i) {
        const auto nextValue = quantize(lambda(events[i].value));
        const auto difference = std::abs(nextValue - lastValue);
        const auto length = min(events[i].delay, maxDelay) - lastDelay;

        if (difference < step) {
            fill<float>(envelope.subspan(lastDelay, length), lastValue);
            lastValue = nextValue;
            lastDelay += length;
            continue;
        }

        const auto numSteps = static_cast<int>(difference / step);
        const auto stepLength = static_cast<int>(length / numSteps);
        for (int i = 0; i < numSteps; ++i) {
            fill<float>(envelope.subspan(lastDelay, stepLength), lastValue);
            lastValue += lastValue <= nextValue ? step : -step;
            lastDelay += stepLength;
        }
    }
    fill<float>(envelope.subspan(lastDelay), lastValue);
}

template <class F>
void multiplicativeEnvelope(const EventVector& events, absl::Span<float> envelope, F&& lambda)
{
    ASSERT(events.size() > 0);
    ASSERT(events[0].delay == 0);

    if (envelope.size() == 0)
        return;
    const auto maxDelay = static_cast<int>(envelope.size() - 1);

    auto lastValue = lambda(events[0].value);
    auto lastDelay = events[0].delay;
    for (unsigned i = 1; i < events.size() && lastDelay < maxDelay; ++i) {
        const auto length = min(events[i].delay, maxDelay) - lastDelay;
        const auto nextValue = lambda(events[i].value);
        const auto step = std::exp((std::log(nextValue) - std::log(lastValue)) / length);
        multiplicativeRamp<float>(envelope.subspan(lastDelay, length), lastValue, step);
        lastValue = nextValue;
        lastDelay += length;
    }
    fill<float>(envelope.subspan(lastDelay), lastValue);
}

template <class F>
void multiplicativeEnvelope(const EventVector& events, absl::Span<float> envelope, F&& lambda, float step)
{
    ASSERT(events.size() > 0);
    ASSERT(events[0].delay == 0);
    ASSERT(step != 0.0f);

    if (envelope.size() == 0)
        return;
    const auto maxDelay = static_cast<int>(envelope.size() - 1);

    const auto logStep = std::log(step);
    // If we assume that a = b.q^r for b in (1, q) then
    // log a     log b
    // -----  =  -----  +  r
    // log q     log q
    // and log(b)\log(q) is between 0 and 1.
    auto quantize = [logStep](float value) -> float {
        if (value > 1)
            return std::exp(logStep * std::floor(std::log(value) / logStep));
        else
            return std::exp(logStep * std::ceil(std::log(value) / logStep));
    };

    auto lastValue = quantize(lambda(events[0].value));
    auto lastDelay = events[0].delay;
    for (unsigned i = 1; i < events.size() && lastDelay < maxDelay; ++i) {
        const auto length = min(events[i].delay, maxDelay) - lastDelay;
        const auto nextValue = quantize(lambda(events[i].value));
        const auto difference = nextValue > lastValue ? nextValue / lastValue : lastValue / nextValue;

        if (difference < step) {
            fill<float>(envelope.subspan(lastDelay, length), lastValue);
            lastValue = nextValue;
            lastDelay += length;
            continue;
        }

        const auto numSteps = static_cast<int>(std::log(difference) / logStep);
        const auto stepLength = static_cast<int>(length / numSteps);
        for (int i = 0; i < numSteps; ++i) {
            fill<float>(envelope.subspan(lastDelay, stepLength), lastValue);
            lastValue = nextValue > lastValue ? lastValue * step : lastValue / step;
            lastDelay += stepLength;
        }
    }
    fill<float>(envelope.subspan(lastDelay), lastValue);
}

template<class F>
void linearModifier(const sfz::Resources& resources, absl::Span<float> span, const sfz::CCData<sfz::Modifier>& ccData, F&& lambda)
{
    const auto events = resources.midiState.getCCEvents(ccData.cc);
    const auto curve = resources.curves.getCurve(ccData.data.curve);
    if (ccData.data.steps < 2) {
        linearEnvelope(events, span, [&ccData, &curve, &lambda](float x) {
            return lambda(curve.evalNormalized(x) * ccData.data.value);
        });
    } else {
        const float stepSize { ccData.data.value / (ccData.data.steps - 1) };
        linearEnvelope(events, span, [&ccData, &curve, &lambda](float x) {
            return lambda(curve.evalNormalized(x) * ccData.data.value);
        }, stepSize);
    }
}

template<class F>
void multiplicativeModifier(const sfz::Resources& resources, absl::Span<float> span, const sfz::CCData<sfz::Modifier>& ccData, F&& lambda)
{
    const auto events = resources.midiState.getCCEvents(ccData.cc);
    const auto curve = resources.curves.getCurve(ccData.data.curve);
    if (ccData.data.steps < 2) {
        multiplicativeEnvelope(events, span, [&ccData, &curve, &lambda](float x) {
            return lambda(curve.evalNormalized(x) * ccData.data.value);
        });
    } else {
        // FIXME: not sure about this step size for multiplicative envelopes
        const float stepSize { lambda(ccData.data.value / (ccData.data.steps - 1)) };
        multiplicativeEnvelope(events, span, [&ccData, &curve, &lambda](float x) {
            return lambda(curve.evalNormalized(x) * ccData.data.value);
        }, stepSize);
    }
}

inline void linearModifier(const sfz::Resources& resources, absl::Span<float> span, const sfz::CCData<sfz::Modifier>& ccData)
{
    linearModifier(resources, span, ccData, [](float x) { return x; });
}
}
