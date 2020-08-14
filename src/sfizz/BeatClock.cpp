// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "BeatClock.h"
#include "Config.h"
#include "Debug.h"
#include <iostream>
#include <cmath>

namespace sfz {

bool TimeSignature::operator==(const TimeSignature& other) const
{
    return beatsPerBar == other.beatsPerBar && beatUnit == other.beatUnit;
}

bool TimeSignature::operator!=(const TimeSignature& other) const
{
    return !operator==(other);
}

///
BBT BBT::toSignature(TimeSignature oldSig, TimeSignature newSig) const
{
    double beatsInOldSig = toBeats(oldSig);
    double beatsInNewSig = beatsInOldSig * newSig.beatUnit / oldSig.beatUnit;
    return BBT::fromBeats(newSig, beatsInNewSig);
}

double BBT::toBeats(TimeSignature sig) const
{
    return beat + bar * sig.beatsPerBar;
}

BBT BBT::fromBeats(TimeSignature sig, double beats)
{
    int newBar = static_cast<int>(beats / sig.beatsPerBar);
    double newBeat = beats - newBar * sig.beatsPerBar;
    return BBT(newBar, newBeat);
}

double BBT::toBars(TimeSignature sig) const
{
    return bar + beat / sig.beatsPerBar;
}

///
constexpr int BeatClock::resolution;

auto BeatClock::quantize(double beats) -> qbeats_t
{
    double d = beats * (1 << resolution);
    d = std::copysign(0.5 + std::fabs(d), d);
    return static_cast<qbeats_t>(d);
}

template <class T>
T BeatClock::dequantize(qbeats_t qbeats)
{
    return qbeats / static_cast<T>(1 << resolution);
}

///
BeatClock::BeatClock()
{
    setSampleRate(config::defaultSampleRate);
    setSamplesPerBlock(config::defaultSamplesPerBlock);
}

void BeatClock::clear()
{
    beatsPerSecond_ = 2.0;
    timeSig_ =  { 4, 4 };
    isPlaying_ = false;

    lastHostPos_ = { 0, 0 };
    lastClientPos_ = { 0, 0 };
}

void BeatClock::beginCycle(unsigned numFrames)
{
    currentCycleFrames_ = numFrames;
    currentCycleFill_ = 0;
    currentCycleStartPos_ = lastClientPos_;
}

void BeatClock::endCycle()
{
    fillBufferUpTo(currentCycleFrames_);
}

void BeatClock::setSampleRate(double sampleRate)
{
    samplePeriod_ = 1.0 / sampleRate;
}

void BeatClock::setSamplesPerBlock(unsigned samplesPerBlock)
{
    runningBeat_.resize(samplesPerBlock);
    runningBeatsPerBar_.resize(samplesPerBlock);
}

void BeatClock::setTempo(unsigned delay, double secondsPerBeat)
{
    fillBufferUpTo(delay);

    beatsPerSecond_ = 1.0 / secondsPerBeat;
}

void BeatClock::setTimeSignature(unsigned delay, TimeSignature newSig)
{
    fillBufferUpTo(delay);

    if (!newSig.valid()) {
        CHECKFALSE;
        return;
    }

    TimeSignature oldSig = timeSig_;
    if (oldSig == newSig)
        return;

    timeSig_ = newSig;

    // convert time to new signature
    lastHostPos_ = lastHostPos_.toSignature(oldSig, newSig);
    lastClientPos_ = lastClientPos_.toSignature(oldSig, newSig);
}

void BeatClock::setTimePosition(unsigned delay, BBT newPos)
{
    fillBufferUpTo(delay);

    lastHostPos_ = newPos;

    // apply host position in the next frame
    mustApplyHostPos_ = true;
}

void BeatClock::setPlaying(unsigned delay, bool playing)
{
    fillBufferUpTo(delay);

    isPlaying_ = playing;
}

absl::Span<const int> BeatClock::getRunningBeat()
{
    fillBufferUpTo(currentCycleFrames_);

    return absl::MakeConstSpan(runningBeat_.data(), currentCycleFrames_);
}

absl::Span<const int> BeatClock::getRunningBeatsPerBar()
{
    fillBufferUpTo(currentCycleFrames_);

    return absl::MakeConstSpan(runningBeatsPerBar_.data(), currentCycleFrames_);
}

void BeatClock::fillBufferUpTo(unsigned delay)
{
    int *beatData = runningBeat_.data();
    int *beatsPerBarData = runningBeatsPerBar_.data();
    unsigned fill = currentCycleFill_;

    const TimeSignature sig = timeSig_;
    for (unsigned i = fill; i < delay; ++i)
        beatsPerBarData[i] = sig.beatsPerBar;

    if (!isPlaying_) {
        for (; fill < delay; ++fill)
            beatData[fill] = 0;
        currentCycleFill_ = fill;
        return;
    }

    BBT clientPos = lastClientPos_;
    const double beatsPerFrame = beatsPerSecond_ * samplePeriod_;

    const BBT hostPos = lastHostPos_;
    bool mustApplyHostPos = mustApplyHostPos_;

    for (; fill < delay; ++fill) {
        clientPos = BBT::fromBeats(sig, clientPos.toBeats(sig) + beatsPerFrame);
        clientPos = mustApplyHostPos ? hostPos : clientPos;
        mustApplyHostPos = false;

        // quantization to nearest for prevention of rounding errors
        beatData[fill] = dequantize<int>(quantize(clientPos.toBeats(sig)));

#if 0
        BBT oldClientPos = clientPos;

        // quantization to nearest for prevention of rounding errors
        qbeats_t oldQbeats = quantize(oldClientPos.toBeats(sig));
        qbeats_t qbeats = quantize(clientPos.toBeats(sig));

        int oldBeatNumber = dequantize<int>(oldQbeats);
        int beatNumber = dequantize<int>(qbeats);

        int beatIncrement = std::max(0, beatNumber - oldBeatNumber);
        int beatDistanceToNextBar = sig.beatsPerBar - (oldBeatNumber % sig.beatsPerBar);
        int barIncrement = (beatIncrement < beatDistanceToNextBar) ? 0 :
            (1 + (beatIncrement - beatDistanceToNextBar) / sig.beatsPerBar);

        beatData[fill] = beatIncrement;
        barData[fill] = barIncrement;
#endif
    }

    currentCycleFill_ = fill;
    lastClientPos_ = clientPos;
    mustApplyHostPos_ = mustApplyHostPos;
}

} // namespace sfz

std::ostream& operator<<(std::ostream& os, const sfz::BBT& pos)
{
    return os << pos.bar << ':' << std::fixed << pos.beat;
}

std::ostream& operator<<(std::ostream& os, const sfz::TimeSignature& sig)
{
    return os << sig.beatsPerBar << '/' << sig.beatUnit;
}
