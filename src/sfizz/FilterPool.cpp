#include "FilterPool.h"
#include "SIMDHelpers.h"
#include "absl/algorithm/container.h"
#include "AtomicGuard.h"
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

sfz::FilterHolder::FilterHolder(const MidiState& midiState)
: midiState(midiState)
{

}

void sfz::FilterHolder::reset()
{
    filter.clear();
}

void sfz::FilterHolder::setup(const FilterDescription& description, unsigned numChannels, int noteNumber, uint8_t velocity)
{
    reset();
    this->description = &description;
    filter.setType(description.type);
    filter.setChannels(numChannels);

    baseCutoff = description.cutoff;
    if (description.random != 0) {
       dist.param(filterRandomDist::param_type(0, description.random));
       baseCutoff *= centsFactor(dist(Random::randomGenerator));
    }
    const auto keytrack = description.keytrack * (noteNumber - description.keycenter);
    baseCutoff *= centsFactor(keytrack);
    const auto veltrack = description.veltrack * normalizeVelocity(velocity);
    baseCutoff *= centsFactor(veltrack);
    baseCutoff = Default::filterCutoffRange.clamp(baseCutoff);

    baseGain = description.gain;
    baseResonance = description.resonance;
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
    lastCutoff = baseCutoff;
    for (auto& mod: description->cutoffCC) {
        lastCutoff *= centsFactor(midiState.getCCValue(mod.first) * mod.second);
    }
    lastCutoff = Default::filterCutoffRange.clamp(lastCutoff);

    lastResonance = baseResonance;
    for (auto& mod: description->resonanceCC) {
        lastResonance += midiState.getCCValue(mod.first) * mod.second;
    }
    lastResonance = Default::filterResonanceRange.clamp(lastResonance);

    lastGain = baseGain;
    for (auto& mod: description->gainCC) {
        lastGain += midiState.getCCValue(mod.first) * mod.second;
    }
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

sfz::FilterHolderPtr sfz::FilterPool::getFilter(const FilterDescription& description, unsigned numChannels, int noteNumber, uint8_t velocity)
{
    AtomicGuard guard { givingOutFilters };
    if (!canGiveOutFilters)
        return {};

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
    AtomicDisabler disabler { canGiveOutFilters };

    while(givingOutFilters)
        std::this_thread::sleep_for(1ms);

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
