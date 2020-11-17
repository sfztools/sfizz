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
    prepared = false;
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

    gainTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqGain, region.id, eqId));
    bandwidthTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqBandwidth, region.id, eqId));
    frequencyTarget = resources.modMatrix.findTarget(ModKey::createNXYZ(ModId::EqFrequency, region.id, eqId));

    // Disables smoothing of the parameters on the first call
    prepared = false;
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

    if (!prepared) {
        eq->prepare(frequencySpan->front(), bandwidthSpan->front(), gainSpan->front());
        prepared = true;
    }

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
