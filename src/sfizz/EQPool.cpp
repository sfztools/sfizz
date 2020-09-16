#include "EQPool.h"
#include <thread>
#include "absl/algorithm/container.h"
#include "SIMDHelpers.h"
#include "SwapAndPop.h"

sfz::EQHolder::EQHolder(const Resources& resources)
: resources(resources)
{
    eq = absl::make_unique<FilterEq>();
    eq->init(config::defaultSampleRate);
}

void sfz::EQHolder::reset()
{
    eq->clear();
}

void sfz::EQHolder::setup(const Region& region, unsigned eqId, float velocity)
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);
    ASSERT(eqId < region.equalizers.size());

    this->description = &region.equalizers[eqId];
    eq->setType(description->type);
    eq->setChannels(region.isStereo() ? 2 : 1);

    // Setup the base values
    baseFrequency = description->frequency + velocity * description->vel2frequency;
    baseBandwidth = description->bandwidth;
    baseGain = description->gain + velocity * description->vel2gain;

    // Setup the modulated values
    float lastFrequency = baseFrequency;
    for (const auto& mod : description->frequencyCC)
        lastFrequency += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastFrequency = Default::eqFrequencyRange.clamp(lastFrequency);

    float lastBandwidth = baseBandwidth;
    for (const auto& mod : description->bandwidthCC)
        lastBandwidth += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastBandwidth = Default::eqBandwidthRange.clamp(lastBandwidth);

    float lastGain = baseGain;
    for (const auto& mod : description->gainCC)
        lastGain += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastGain = Default::filterGainRange.clamp(lastGain);

    // Initialize the EQ
    eq->prepare(lastFrequency, lastBandwidth, lastGain);
}

void sfz::EQHolder::process(const float** inputs, float** outputs, unsigned numFrames)
{
    auto justCopy = [&]() {
        for (unsigned channelIdx = 0; channelIdx < eq->channels(); channelIdx++)
            copy<float>({ inputs[channelIdx], numFrames }, { outputs[channelIdx], numFrames });
    };

    if (description == nullptr) {
        justCopy();
        return;
    }

    // TODO: Once the midistate envelopes are done, add modulation in there!
    // For now we take the last value
    float lastFrequency = baseFrequency;
    for (const auto& mod : description->frequencyCC)
        lastFrequency += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastFrequency = Default::eqFrequencyRange.clamp(lastFrequency);

    float lastBandwidth = baseBandwidth;
    for (const auto& mod : description->bandwidthCC)
        lastBandwidth += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastBandwidth = Default::eqBandwidthRange.clamp(lastBandwidth);

    float lastGain = baseGain;
    for (const auto& mod : description->gainCC)
        lastGain += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastGain = Default::filterGainRange.clamp(lastGain);

    if (lastGain == 0.0f) {
        justCopy();
        return;
    }

    eq->process(inputs, outputs, lastFrequency, lastBandwidth, lastGain, numFrames);
}

void sfz::EQHolder::setSampleRate(float sampleRate)
{
    eq->init(static_cast<double>(sampleRate));
}
