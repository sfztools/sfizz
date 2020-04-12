#include "EQPool.h"
#include <thread>
#include "absl/algorithm/container.h"
#include "SIMDHelpers.h"

sfz::EQHolder::EQHolder(const MidiState& state)
:midiState(state)
{

}

void sfz::EQHolder::reset()
{
    eq.clear();
}

void sfz::EQHolder::setup(const EQDescription& description, unsigned numChannels, float velocity)
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);
    eq.setType(description.type);
    eq.setChannels(numChannels);
    this->description = &description;

    // Setup the base values
    baseFrequency = description.frequency + velocity * description.vel2frequency;
    baseBandwidth = description.bandwidth;
    baseGain = description.gain + velocity * description.vel2gain;

    // Setup the modulated values
    lastFrequency = baseFrequency;
    for (const auto& mod : description.frequencyCC)
        lastFrequency += midiState.getCCValue(mod.cc) * mod.value;
    lastFrequency = Default::eqFrequencyRange.clamp(lastFrequency);

    lastBandwidth = baseBandwidth;
    for (const auto& mod : description.bandwidthCC)
        lastBandwidth += midiState.getCCValue(mod.cc) * mod.value;
    lastBandwidth = Default::eqBandwidthRange.clamp(lastBandwidth);

    lastGain = baseGain;
    for (const auto& mod : description.gainCC)
        lastGain += midiState.getCCValue(mod.cc) * mod.value;
    lastGain = Default::filterGainRange.clamp(lastGain);

    // Initialize the EQ
    eq.prepare(lastFrequency, lastBandwidth, lastGain);
}

void sfz::EQHolder::process(const float** inputs, float** outputs, unsigned numFrames)
{
    auto justCopy = [&]() {
        for (unsigned channelIdx = 0; channelIdx < eq.channels(); channelIdx++)
            copy<float>({ inputs[channelIdx], numFrames }, { outputs[channelIdx], numFrames });
    };

    if (description == nullptr) {
        justCopy();
        return;
    }

    // TODO: Once the midistate envelopes are done, add modulation in there!
    // For now we take the last value
    lastFrequency = baseFrequency;
    for (const auto& mod : description->frequencyCC)
        lastFrequency += midiState.getCCValue(mod.cc) * mod.value;
    lastFrequency = Default::eqFrequencyRange.clamp(lastFrequency);

    lastBandwidth = baseBandwidth;
    for (const auto& mod : description->bandwidthCC)
        lastBandwidth += midiState.getCCValue(mod.cc) * mod.value;
    lastBandwidth = Default::eqBandwidthRange.clamp(lastBandwidth);

    lastGain = baseGain;
    for (const auto& mod : description->gainCC)
        lastGain += midiState.getCCValue(mod.cc) * mod.value;
    lastGain = Default::filterGainRange.clamp(lastGain);

    if (lastGain == 0.0f) {
        justCopy();
        return;
    }

    eq.process(inputs, outputs, lastFrequency, lastBandwidth, lastGain, numFrames);
}
float sfz::EQHolder::getLastFrequency() const
{
    return lastFrequency;
}
float sfz::EQHolder::getLastBandwidth() const
{
    return lastBandwidth;
}
float sfz::EQHolder::getLastGain() const
{
    return lastGain;
}
void sfz::EQHolder::setSampleRate(float sampleRate)
{
    eq.init(static_cast<double>(sampleRate));
}

sfz::EQPool::EQPool(const MidiState& state, int numEQs)
: midiState(state)
{
    setnumEQs(numEQs);
}

sfz::EQHolderPtr sfz::EQPool::getEQ(const EQDescription& description, unsigned numChannels, float velocity)
{
    const std::unique_lock<std::mutex> lock { eqGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return {};


    auto eq = absl::c_find_if(eqs, [](const EQHolderPtr& holder) {
        return holder.use_count() == 1;
    });

    if (eq == eqs.end())
        return {};

    (**eq).setup(description, numChannels, velocity);
    return *eq;
}

size_t sfz::EQPool::getActiveEQs() const
{
    return absl::c_count_if(eqs, [](const EQHolderPtr& holder) {
        return holder.use_count() > 1;
    });
}

size_t sfz::EQPool::setnumEQs(size_t numEQs)
{
    const std::lock_guard<std::mutex> eqLock { eqGuard };

    auto eqIterator = eqs.begin();
    auto eqSentinel = eqs.rbegin();
    while (eqIterator < eqSentinel.base()) {
        if (eqIterator->use_count() == 1) {
            std::iter_swap(eqIterator, eqSentinel);
            ++eqSentinel;
        } else {
            ++eqIterator;
        }
    }

    eqs.resize(std::distance(eqs.begin(), eqSentinel.base()));
    for (size_t i = eqs.size(); i < numEQs; ++i) {
        eqs.emplace_back(std::make_shared<EQHolder>(midiState));
        eqs.back()->setSampleRate(sampleRate);
    }

    return eqs.size();
}
void sfz::EQPool::setSampleRate(float sampleRate)
{
    for (auto& eq: eqs)
        eq->setSampleRate(sampleRate);
}
