#include "FilterPool.h"
#include "SIMDHelpers.h"
#include "absl/algorithm/container.h"
#include "Defer.h"
#include <thread>
#include <chrono>

sfz::FilterHolder::FilterHolder(const MidiState& midiState)
: midiState(midiState)
{

}

void sfz::FilterHolder::reset()
{
    filter.clear();
}

void sfz::FilterHolder::setup(const FilterDescription& description, unsigned numChannels, int noteNumber, float velocity)
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    this->description = &description;
    filter.setType(description.type);
    filter.setChannels(numChannels);

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
    lastCutoff = baseCutoff;
    for (const auto& mod : description.cutoffCC)
        lastCutoff *= centsFactor(midiState.getCCValue(mod.cc) * mod.value);
    lastCutoff = Default::filterCutoffRange.clamp(lastCutoff);

    lastResonance = baseResonance;
    for (const auto& mod : description.resonanceCC)
        lastResonance += midiState.getCCValue(mod.cc) * mod.value;
    lastResonance = Default::filterResonanceRange.clamp(lastResonance);

    lastGain = baseGain;
    for (const auto& mod : description.gainCC)
        lastGain += midiState.getCCValue(mod.cc) * mod.value;
    lastGain = Default::filterGainRange.clamp(lastGain);

    // Initialize the filter
    filter.prepare(lastCutoff, lastResonance, lastGain);
}

void sfz::FilterHolder::process(const float** inputs, float** outputs, unsigned numFrames)
{
    if (description == nullptr) {
        for (unsigned channelIdx = 0; channelIdx < filter.channels(); channelIdx++)
            copy<float>({ inputs[channelIdx], numFrames }, { outputs[channelIdx], numFrames });
        return;
    }

    // TODO: Once the midistate envelopes are done, add modulation in there!
    // For now we take the last value
    // TODO: the template deduction could be automatic here?
    lastCutoff = baseCutoff;
    for (const auto& mod : description->cutoffCC)
        lastCutoff *= centsFactor(midiState.getCCValue(mod.cc) * mod.value);
    lastCutoff = Default::filterCutoffRange.clamp(lastCutoff);

    lastResonance = baseResonance;
    for (const auto& mod : description->resonanceCC)
        lastResonance += midiState.getCCValue(mod.cc) * mod.value;
    lastResonance = Default::filterResonanceRange.clamp(lastResonance);

    lastGain = baseGain;
    for (const auto& mod : description->gainCC)
        lastGain += midiState.getCCValue(mod.cc) * mod.value;
    lastGain = Default::filterGainRange.clamp(lastGain);

    filter.process(inputs, outputs, lastCutoff, lastResonance, lastGain, numFrames);
}

float sfz::FilterHolder::getLastCutoff() const
{
    return lastCutoff;
}
float sfz::FilterHolder::getLastResonance() const
{
    return lastResonance;
}
float sfz::FilterHolder::getLastGain() const
{
    return lastGain;
}

sfz::FilterPool::FilterPool(const MidiState& state, int numFilters)
: midiState(state)
{
    setNumFilters(numFilters);
}

sfz::FilterHolderPtr sfz::FilterPool::getFilter(const FilterDescription& description, unsigned numChannels, int noteNumber, float velocity)
{
    if (!filterGuard.try_lock())
        return {};
    DEFER { filterGuard.unlock(); };

    auto filter = absl::c_find_if(filters, [](const FilterHolderPtr& holder) {
        return holder.use_count() == 1;
    });

    if (filter == filters.end())
        return {};

    (**filter).setup(description, numChannels, noteNumber, velocity);
    return *filter;
}

size_t sfz::FilterPool::getActiveFilters() const
{
    return absl::c_count_if(filters, [](const FilterHolderPtr& holder) {
        return holder.use_count() > 1;
    });
}

size_t sfz::FilterPool::setNumFilters(size_t numFilters)
{
    const std::lock_guard filterLock { filterGuard };

    auto filterIterator = filters.begin();
    auto filterSentinel = filters.rbegin();
    while (filterIterator < filterSentinel.base()) {
        if (filterIterator->use_count() == 1) {
            std::iter_swap(filterIterator, filterSentinel);
            ++filterSentinel;
        } else {
            ++filterIterator;
        }
    }

    filters.resize(std::distance(filters.begin(), filterSentinel.base()));
    for (size_t i = filters.size(); i < numFilters; ++i) {
        filters.emplace_back(std::make_shared<FilterHolder>(midiState));
        filters.back()->setSampleRate(sampleRate);
    }

    return filters.size();
}

void sfz::FilterPool::setSampleRate(float sampleRate)
{
    for (auto& filter: filters)
        filter->setSampleRate(sampleRate);
}

void sfz::FilterHolder::setSampleRate(float sampleRate)
{
    filter.init(static_cast<double>(sampleRate));
}
