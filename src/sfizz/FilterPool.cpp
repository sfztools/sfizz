#include "FilterPool.h"
#include "SIMDHelpers.h"
#include "absl/algorithm/container.h"

sfz::FilterHolder::FilterHolder(const MidiState& midiState)
: midiState(midiState)
{
    filter.setChannels(2);
}

void sfz::FilterHolder::reset()
{
    filter.clear();
}

void sfz::FilterHolder::setup(const FilterDescription& description, int noteNumber, uint8_t velocity)
{
    reset();
    this->description = &description;
    filter.setType(description.type);

    baseCutoff = description.cutoff;
    if (description.random != 0) {
       dist.param(filterRandomDist::param_type(0, description.random));
       baseCutoff *= centsFactor(dist(Random::randomGenerator));
    }
    const auto keytrack = description.keytrack * (noteNumber - description.keycenter);
    baseCutoff *= centsFactor(keytrack);
    const auto veltrack = description.veltrack * velocity;
    baseCutoff *= centsFactor(veltrack);

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
    auto cutoff = baseCutoff;
    for (auto& mod: description->cutoffCC) {
        cutoff *= centsFactor(midiState.getCCValue(mod.first) * mod.second);
    }

    auto resonance = baseResonance;
    for (auto& mod: description->resonanceCC) {
        resonance += midiState.getCCValue(mod.first) * mod.second;
    }

    auto gain = baseGain;
    for (auto& mod: description->gainCC) {
        gain += midiState.getCCValue(mod.first) * mod.second;
    }

    filter.process(inputs, outputs, cutoff, resonance, gain,numFrames);
}


sfz::FilterPool::FilterPool(const MidiState& state, int numFilters)
: midiState(state)
{
    setNumFilters(numFilters);
}

sfz::FilterHolderPtr sfz::FilterPool::getFilter(const FilterDescription& description, int noteNumber, uint8_t velocity)
{
    auto filter = absl::c_find_if(filters, [](const FilterHolderPtr& holder) {
        return holder.use_count() == 1;
    });

    if (filter == filters.end())
        return {};

    (**filter).setup(description, noteNumber, velocity);
    return *filter;
}

int sfz::FilterPool::getActiveFilters() const
{
    return absl::c_count_if(filters, [](const FilterHolderPtr& holder) {
        return holder.use_count() > 1;
    });
}

void sfz::FilterPool::setNumFilters(int numFilters)
{
    filters.clear();
    filters.reserve(numFilters);
    for (int i = 0; i < numFilters; ++i)
        filters.emplace_back(std::make_shared<FilterHolder>(midiState));
}
