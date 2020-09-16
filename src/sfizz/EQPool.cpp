#include "EQPool.h"
#include <thread>
#include "absl/algorithm/container.h"
#include "SIMDHelpers.h"
#include "SwapAndPop.h"

sfz::EQHolder::EQHolder(Resources& resources)
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

    gainTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqGain, region.id, eqId));
    bandwidthTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqBandwidth, region.id, eqId));
    frequencyTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqFrequency, region.id, eqId));

    // Initialize the EQ
    DBG(baseFrequency << " " << baseBandwidth << " " << baseGain);
    eq->prepare(lastFrequency, lastBandwidth, lastGain);
}

void sfz::EQHolder::process(const float** inputs, float** outputs, unsigned numFrames)
{
    if (description == nullptr) {
        for (unsigned channelIdx = 0; channelIdx < eq->channels(); channelIdx++)
            copy<float>({ inputs[channelIdx], numFrames }, { outputs[channelIdx], numFrames });
        return;
    }

    ModMatrix& mm = resources.modMatrix;
    auto frequencySpan = resources.bufferPool.getBuffer(numFrames);
    auto bandwidthSpan = resources.bufferPool.getBuffer(numFrames);
    auto gainSpan = resources.bufferPool.getBuffer(numFrames);

    if (!frequencySpan || !bandwidthSpan || !gainSpan)
        return;

    fill<float>(*frequencySpan, baseFrequency);
    if (float* mod = mm.getModulation(frequencyTarget))
        add<float>(absl::Span<float>(mod, numFrames), *frequencySpan);

    fill<float>(*bandwidthSpan, baseBandwidth);
    if (float* mod = mm.getModulation(bandwidthTarget))
        add<float>(absl::Span<float>(mod, numFrames), *bandwidthSpan);

    fill<float>(*gainSpan, baseGain);
    if (float* mod = mm.getModulation(gainTarget))
        add<float>(absl::Span<float>(mod, numFrames), *gainSpan);

    DBG(frequencySpan->back() << " " << bandwidthSpan->back() << " " << gainSpan->back());

    eq->processModulated(
        inputs,
        outputs,
        frequencySpan->data(),
        bandwidthSpan->data(),
        gainSpan->data(),
        numFrames
    );
}

void sfz::EQHolder::setSampleRate(float sampleRate)
{
    eq->init(static_cast<double>(sampleRate));
}
