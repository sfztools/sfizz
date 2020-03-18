// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Effects.h"
#include "AudioSpan.h"
#include "Opcode.h"
#include "SIMDHelpers.h"
#include "Config.h"
#include "effects/Nothing.h"
#include "effects/Filter.h"
#include "effects/Eq.h"
#include "effects/Apan.h"
#include "effects/Lofi.h"
#include "effects/Limiter.h"
#include "effects/Rectify.h"
#include <algorithm>

namespace sfz {

void EffectFactory::registerStandardEffectTypes()
{
    // TODO
    registerEffectType("filter", fx::Filter::makeInstance);
    registerEffectType("eq", fx::Eq::makeInstance);
    registerEffectType("apan", fx::Apan::makeInstance);
    registerEffectType("lofi", fx::Lofi::makeInstance);
    registerEffectType("limiter", fx::Limiter::makeInstance);

    // extensions (book)
    registerEffectType("rectify", fx::Rectify::makeInstance);
}

void EffectFactory::registerEffectType(absl::string_view name, Effect::MakeInstance& make)
{
    FactoryEntry ent;
    ent.name = std::string(name);
    ent.make = &make;
    _entries.push_back(std::move(ent));
}

std::unique_ptr<Effect> EffectFactory::makeEffect(absl::Span<const Opcode> members)
{
    const Opcode* opcode = nullptr;

    for (auto it = members.rbegin(); it != members.rend() && !opcode; ++it) {
        if (it->lettersOnlyHash == hash("type"))
            opcode = &*it;
    }

    if (!opcode) {
        DBG("The effect does not specify a type");
        return absl::make_unique<sfz::fx::Nothing>();
    }

    const absl::string_view type = opcode->value;

    const auto it = absl::c_find_if(_entries, [&](const FactoryEntry& entry) { return entry.name == type; });
    if (it == _entries.end()) {
        DBG("Unsupported effect type: " << type);
        return absl::make_unique<sfz::fx::Nothing>();
    }

    auto fx = it->make(members);
    if (!fx) {
        DBG("Could not instantiate effect of type: " << type);
        return absl::make_unique<sfz::fx::Nothing>();
    }

    return fx;
}

///
EffectBus::EffectBus()
{
}

EffectBus::~EffectBus()
{
}

void EffectBus::addEffect(std::unique_ptr<Effect> fx)
{
    _effects.emplace_back(std::move(fx));
}

void EffectBus::clearInputs(unsigned nframes)
{
    AudioSpan<float>(_inputs).first(nframes).fill(0.0f);
    AudioSpan<float>(_outputs).first(nframes).fill(0.0f);
}

void EffectBus::addToInputs(const float* const addInput[], float addGain, unsigned nframes)
{
    if (addGain == 0)
        return;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        absl::Span<const float> addIn { addInput[c], nframes };
        sfz::multiplyAdd(addGain, addIn, _inputs.getSpan(c));
    }
}

void EffectBus::setSampleRate(double sampleRate)
{
    for (const auto& effectPtr : _effects)
        effectPtr->setSampleRate(sampleRate);
}

void EffectBus::clear()
{
    for (const auto& effectPtr : _effects)
        effectPtr->clear();
}

void EffectBus::process(unsigned nframes)
{
    size_t numEffects = _effects.size();

    if (numEffects > 0 && hasNonZeroOutput()) {
        _effects[0]->process(
            AudioSpan<float>(_inputs), AudioSpan<float>(_outputs), nframes);
        for (size_t i = 1; i < numEffects; ++i)
            _effects[i]->process(
                AudioSpan<float>(_outputs), AudioSpan<float>(_outputs), nframes);
    } else
        fx::Nothing().process(
            AudioSpan<float>(_inputs), AudioSpan<float>(_outputs), nframes);
}

void EffectBus::mixOutputsTo(float* const mainOutput[], float* const mixOutput[], unsigned nframes)
{
    const float gainToMain = _gainToMain;
    const float gainToMix = _gainToMix;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        auto fxOut = _outputs.getConstSpan(c);
        sfz::multiplyAdd(gainToMain, fxOut, absl::Span<float>(mainOutput[c], nframes));
        sfz::multiplyAdd(gainToMix, fxOut, absl::Span<float>(mixOutput[c], nframes));
    }
}

size_t EffectBus::numEffects() const noexcept
{
    return _effects.size();
}
void EffectBus::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    _inputs.resize(samplesPerBlock);
    _outputs.resize(samplesPerBlock);

    for (const auto& effectPtr : _effects)
        effectPtr->setSamplesPerBlock(samplesPerBlock);
}

} // namespace sfz
