// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Controller.h"
#include "../ModKey.h"
#include "../../Smoothers.h"
#include "../../ModifierHelpers.h"
#include "../../Resources.h"
#include "../../Config.h"
#include "../../Debug.h"
#include <absl/container/flat_hash_map.h>

namespace sfz {

struct ControllerSource::Impl {
    float getLastTransformedValue(uint16_t cc, uint8_t curve) const noexcept;
    double sampleRate_ = config::defaultSampleRate;
    Resources* res_ = nullptr;
    absl::flat_hash_map<ModKey, Smoother> smoother_;
};

ControllerSource::ControllerSource(Resources& res)
    : impl_(new Impl)
{
    impl_->res_ = &res;
}

ControllerSource::~ControllerSource()
{
}

float ControllerSource::Impl::getLastTransformedValue(uint16_t cc, uint8_t curveIndex) const noexcept
{
    ASSERT(res_);
    const Curve& curve = res_->curves.getCurve(curveIndex);
    const auto lastCCValue = res_->midiState.getCCValue(cc);
    return curve.evalNormalized(lastCCValue);
}

void ControllerSource::resetSmoothers()
{
    for (auto& item : impl_->smoother_) {
        const ModKey::Parameters p = item.first.parameters();
        item.second.reset(impl_->getLastTransformedValue(p.cc, p.curve));
    }
}

void ControllerSource::setSampleRate(double sampleRate)
{
    if (impl_->sampleRate_ == sampleRate)
        return;

    impl_->sampleRate_ = sampleRate;

    for (auto& item : impl_->smoother_) {
        const ModKey::Parameters p = item.first.parameters();
        item.second.setSmoothing(p.smooth, sampleRate);
    }
}

void ControllerSource::setSamplesPerBlock(unsigned count)
{
    (void)count;
}

void ControllerSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    (void)voiceId;
    (void)delay;

    const ModKey::Parameters p = sourceKey.parameters();
    if (p.smooth > 0) {
        Smoother s;
        s.setSmoothing(p.smooth, impl_->sampleRate_);
        s.reset(impl_->getLastTransformedValue(p.cc, p.curve));
        impl_->smoother_[sourceKey] = s;
    }
    else {
        impl_->smoother_.erase(sourceKey);
    }
}

void ControllerSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    (void)voiceId;

    const ModKey::Parameters p = sourceKey.parameters();
    const Resources& res = *impl_->res_;
    const Curve& curve = res.curves.getCurve(p.curve);
    const MidiState& ms = res.midiState;
    const EventVector& events = ms.getCCEvents(p.cc);

    auto transformValue = [p, &curve](float x) {
        return curve.evalNormalized(x);
    };

    if (p.step > 0.0f)
        linearEnvelope(events, buffer, transformValue, p.step);
    else
        linearEnvelope(events, buffer, transformValue);

    auto it = impl_->smoother_.find(sourceKey);
    if (it != impl_->smoother_.end()) {
        Smoother& s = it->second;
        bool canShortcut = events.size() == 1;
        s.process(buffer, buffer, canShortcut);
    }
}

} // namespace sfz
