// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "RegionStateful.h"
#include "ModifierHelpers.h"

namespace sfz {

float getBaseVolumedB(const Region& region, const MidiState& midiState, int noteNumber) noexcept
{
    fast_real_distribution<float> volumeDistribution { 0.0f, region.ampRandom };
    auto baseVolumedB = region.volume + volumeDistribution(Random::randomGenerator);
    baseVolumedB += region.globalVolume;
    baseVolumedB += region.masterVolume;
    baseVolumedB += region.groupVolume;
    if (region.trigger == Trigger::release || region.trigger == Trigger::release_key)
        baseVolumedB -= region.rtDecay * midiState.getNoteDuration(noteNumber);
    return baseVolumedB;
}


uint64_t getOffset(const Region& region, const MidiState& midiState) noexcept
{
    std::uniform_int_distribution<int64_t> offsetDistribution { 0, region.offsetRandom };
    uint64_t finalOffset = region.offset + offsetDistribution(Random::randomGenerator);
    for (const auto& mod: region.offsetCC)
        finalOffset += static_cast<uint64_t>(mod.data * midiState.getCCValue(mod.cc));
    return Default::offset.bounds.clamp(finalOffset);
}

float getDelay(const Region& region, const MidiState& midiState) noexcept
{
    fast_real_distribution<float> delayDistribution { 0, region.delayRandom };
    float finalDelay { region.delay };
    finalDelay += delayDistribution(Random::randomGenerator);
    for (const auto& mod: region.delayCC)
        finalDelay += mod.data * midiState.getCCValue(mod.cc);

    return Default::delay.bounds.clamp(finalDelay);
}

uint32_t getSampleEnd(const Region& region, MidiState& midiState) noexcept
{
    int64_t end = region.sampleEnd;
    for (const auto& mod: region.endCC)
        end += static_cast<int64_t>(mod.data * midiState.getCCValue(mod.cc));

    end = clamp(end, int64_t { 0 }, region.sampleEnd);
    return static_cast<uint32_t>(end);
}

uint32_t loopStart(const Region& region, MidiState& midiState) noexcept
{
    auto start = region.loopRange.getStart();
    for (const auto& mod: region.loopStartCC)
        start += static_cast<int64_t>(mod.data * midiState.getCCValue(mod.cc));

    start = clamp(start, int64_t { 0 }, region.sampleEnd);
    return static_cast<uint32_t>(start);
}

uint32_t loopEnd(const Region& region, MidiState& midiState) noexcept
{
    auto end = region.loopRange.getEnd();
    for (const auto& mod: region.loopEndCC)
        end += static_cast<int64_t>(mod.data * midiState.getCCValue(mod.cc));

    end = clamp(end, int64_t { 0 }, region.sampleEnd);
    return static_cast<uint32_t>(end);
}

float getNoteGain(const Region& region, int noteNumber, float velocity, const MidiState& midiState, const CurveSet& curveSet) noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    float baseGain { 1.0f };

    // Amplitude key tracking
    baseGain *= db2mag(region.ampKeytrack * static_cast<float>(noteNumber - region.ampKeycenter));

    // Crossfades related to the note number
    baseGain *= crossfadeIn(region.crossfadeKeyInRange, noteNumber, region.crossfadeKeyCurve);
    baseGain *= crossfadeOut(region.crossfadeKeyOutRange, noteNumber, region.crossfadeKeyCurve);

    // Amplitude velocity tracking
    baseGain *= velocityCurve(region, velocity, midiState, curveSet);

    // Crossfades related to velocity
    baseGain *= crossfadeIn(region.crossfadeVelInRange, velocity, region.crossfadeVelCurve);
    baseGain *= crossfadeOut(region.crossfadeVelOutRange, velocity, region.crossfadeVelCurve);

    return baseGain;
}

float getCrossfadeGain(const Region& region, const MidiState& midiState) noexcept
{
    float gain { 1.0f };

    // Crossfades due to CC states
    for (const auto& ccData : region.crossfadeCCInRange) {
        const auto ccValue = midiState.getCCValue(ccData.cc);
        const auto crossfadeRange = ccData.data;
        gain *= crossfadeIn(crossfadeRange, ccValue, region.crossfadeCCCurve);
    }

    for (const auto& ccData : region.crossfadeCCOutRange) {
        const auto ccValue = midiState.getCCValue(ccData.cc);
        const auto crossfadeRange = ccData.data;
        gain *= crossfadeOut(crossfadeRange, ccValue, region.crossfadeCCCurve);
    }

    return gain;
}

float velocityCurve(const Region& region, float velocity, const MidiState& midiState, const CurveSet& curveSet) noexcept
{
    ASSERT(velocity >= 0.0f && velocity <= 1.0f);

    float gain;
    if (region.velCurve)
        gain = region.velCurve->evalNormalized(velocity);
    else
        gain = velocity * velocity;

    gain = std::fabs(region.ampVeltrack) * (1.0f - gain);
    gain = (region.ampVeltrack < 0) ? gain : (1.0f - gain);

    return gain;
}

}
