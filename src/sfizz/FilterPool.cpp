#include "FilterPool.h"
#include "SIMDHelpers.h"
#include "SwapAndPop.h"
#include "absl/algorithm/container.h"
#include <thread>
#include <chrono>

sfz::FilterHolder::FilterHolder(const Resources& resources)
: resources(resources)
{
    filter = absl::make_unique<Filter>();
    filter->init(config::defaultSampleRate);
}

void sfz::FilterHolder::reset()
{
    filter->clear();
}

void sfz::FilterHolder::setup(const FilterDescription& description, unsigned numChannels, int noteNumber, float velocity)
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    this->description = &description;
    filter->setType(description.type);
    filter->setChannels(numChannels);

    // Setup the base values
    baseCutoff = description.cutoff;
    if (description.random != 0) {
       dist.param(filterRandomDist::param_type(0, description.random));
       baseCutoff *= centsFactor(dist(Random::randomGenerator));
    }
    const auto keytrack = description.keytrack * (noteNumber - description.keycenter);
    baseCutoff *= centsFactor(keytrack);
    const auto veltrack = static_cast<float>(description.veltrack) * velocity;
    baseCutoff *= centsFactor(veltrack);
    baseCutoff = Default::filterCutoffRange.clamp(baseCutoff);

    baseGain = description.gain;
    baseResonance = description.resonance;

    // Setup the modulated values
    float lastCutoff = baseCutoff;
    for (const auto& mod : description.cutoffCC)
        lastCutoff *= centsFactor(resources.midiState.getCCValue(mod.cc) * mod.data);
    lastCutoff = Default::filterCutoffRange.clamp(lastCutoff);

    float lastResonance = baseResonance;
    for (const auto& mod : description.resonanceCC)
        lastResonance += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastResonance = Default::filterResonanceRange.clamp(lastResonance);

    float lastGain = baseGain;
    for (const auto& mod : description.gainCC)
        lastGain += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastGain = Default::filterGainRange.clamp(lastGain);

    // Initialize the filter
    filter->prepare(lastCutoff, lastResonance, lastGain);
}

void sfz::FilterHolder::process(const float** inputs, float** outputs, unsigned numFrames)
{
    if (description == nullptr) {
        for (unsigned channelIdx = 0; channelIdx < filter->channels(); channelIdx++)
            copy<float>({ inputs[channelIdx], numFrames }, { outputs[channelIdx], numFrames });
        return;
    }

    // TODO: Once the midistate envelopes are done, add modulation in there!
    // For now we take the last value
    // TODO: the template deduction could be automatic here?
    float lastCutoff = baseCutoff;
    for (const auto& mod : description->cutoffCC)
        lastCutoff *= centsFactor(resources.midiState.getCCValue(mod.cc) * mod.data);
    lastCutoff = Default::filterCutoffRange.clamp(lastCutoff);

    float lastResonance = baseResonance;
    for (const auto& mod : description->resonanceCC)
        lastResonance += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastResonance = Default::filterResonanceRange.clamp(lastResonance);

    float lastGain = baseGain;
    for (const auto& mod : description->gainCC)
        lastGain += resources.midiState.getCCValue(mod.cc) * mod.data;
    lastGain = Default::filterGainRange.clamp(lastGain);

    filter->process(inputs, outputs, lastCutoff, lastResonance, lastGain, numFrames);
}


void sfz::FilterHolder::setSampleRate(float sampleRate)
{
    filter->init(static_cast<double>(sampleRate));
}
