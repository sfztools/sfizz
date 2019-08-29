#include "Voice.h"
#include "SIMDHelpers.h"
#include "SfzHelpers.h"

sfz::Voice::Voice(const CCValueArray& ccState)
    : ccState(ccState)
{
}

void sfz::Voice::startVoice(Region* region, int delay, int channel, int number, uint8_t value, sfz::Voice::TriggerType triggerType)
{
    this->triggerType = triggerType;
    triggerNumber = number;
    triggerChannel = channel;
    triggerValue = value;

    this->region = region;

    ASSERT(delay >= 0);
    if (delay < 0)
        delay = 0;

    DBG("Starting voice with " << region->sample);

    state = State::playing;
    speedRatio = static_cast<float>(region->sampleRate / this->sampleRate);
    pitchRatio = region->getBasePitchVariation(number, value);
    baseGain = region->getBaseGain();
    sourcePosition = region->getOffset();
    initialDelay = delay + region->getDelay();
    baseFrequency = midiNoteFrequency(number) * pitchRatio;
    prepareEGEnvelope(delay, value);
}

void sfz::Voice::prepareEGEnvelope(int delay, uint8_t velocity)
{
    auto secondsToSamples = [this](auto timeInSeconds) {
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    egEnvelope.reset(
        secondsToSamples(region->amplitudeEG.getAttack(ccState, velocity)),
        secondsToSamples(region->amplitudeEG.getRelease(ccState, velocity)),
        normalizePercents(region->amplitudeEG.getSustain(ccState, velocity)),
        delay + secondsToSamples(region->amplitudeEG.getDelay(ccState, velocity)),
        secondsToSamples(region->amplitudeEG.getDecay(ccState, velocity)),
        secondsToSamples(region->amplitudeEG.getHold(ccState, velocity)),
        normalizePercents(region->amplitudeEG.getStart(ccState, velocity)));
}

void sfz::Voice::setFileData(std::unique_ptr<StereoBuffer<float>> file)
{
    fileData = std::move(file);
    dataReady.store(true);
}

bool sfz::Voice::isFree() const
{
    return (region == nullptr);
}

void sfz::Voice::registerNoteOff(int delay, int channel, int noteNumber, uint8_t velocity [[maybe_unused]])
{
    if (region == nullptr)
        return;

    if (state != State::playing)
        return;

    if (triggerChannel == channel && triggerNumber == noteNumber) {
        noteIsOff = true;

        if (region->loopMode == SfzLoopMode::one_shot)
            return;

        if (ccState[64] < 63)
            egEnvelope.startRelease(delay);
    }
}

void sfz::Voice::registerCC(int delay, int channel [[maybe_unused]], int ccNumber, uint8_t ccValue)
{
    if (ccNumber == 64 && noteIsOff && ccValue < 63)
        egEnvelope.startRelease(delay);
}

void sfz::Voice::registerPitchWheel(int delay [[maybe_unused]], int channel [[maybe_unused]], int pitch [[maybe_unused]])
{

}

void sfz::Voice::registerAftertouch(int delay [[maybe_unused]], int channel [[maybe_unused]], uint8_t aftertouch [[maybe_unused]])
{

}

void sfz::Voice::registerTempo(int delay [[maybe_unused]], float secondsPerQuarter [[maybe_unused]])
{

}

void sfz::Voice::setSampleRate(float sampleRate)
{
    this->sampleRate = sampleRate;
}

void sfz::Voice::setSamplesPerBlock(int samplesPerBlock)
{
    this->samplesPerBlock = samplesPerBlock;
    tempBuffer1.resize(samplesPerBlock);
    tempBuffer2.resize(samplesPerBlock);
    indexBuffer.resize(samplesPerBlock);
    tempSpan1 = absl::MakeSpan(tempBuffer1);
    tempSpan2 = absl::MakeSpan(tempBuffer2);
    indexSpan = absl::MakeSpan(indexBuffer);
}

void sfz::Voice::renderBlock(StereoSpan<float> buffer)
{
    const auto numSamples = buffer.size();
    ASSERT(static_cast<int>(numSamples) <= samplesPerBlock);
    buffer.fill(0.0f);

    if (state == State::idle || region == nullptr)
        return;

    if (region->isGenerator())
        fillWithGenerator(buffer);
    else
        fillWithData(buffer);

    buffer.applyGain(baseGain);

    auto envelopeSpan = tempSpan1.first(numSamples);
    egEnvelope.getBlock(envelopeSpan);
    buffer.applyGain(envelopeSpan);

    if (!egEnvelope.isSmoothing())
        reset();
}

void sfz::Voice::fillWithData(StereoSpan<float> buffer)
{
    auto source { [&]() {
        if (region->canUsePreloadedData() || !dataReady)
            return StereoSpan<const float>(*region->preloadedData);
        else
            return StereoSpan<const float>(*fileData);
    }() };

    auto indices = indexSpan.first(buffer.size());
    auto jumps = tempSpan1.first(buffer.size());
    auto leftCoeffs = tempSpan1.first(buffer.size());
    auto rightCoeffs = tempSpan2.first(buffer.size());

    ::fill<float>(jumps, pitchRatio * speedRatio);

    if (region->shouldLoop() && region->trueSampleEnd() <= source.size()) {
        floatPosition = ::loopingSFZIndex<float, false>(
            jumps,
            leftCoeffs,
            rightCoeffs,
            indices,
            floatPosition,
            region->trueSampleEnd() - 1,
            region->loopRange.getStart());
    } else {
        floatPosition = ::saturatingSFZIndex<float, false>(
            jumps,
            leftCoeffs,
            rightCoeffs,
            indices,
            floatPosition,
            source.size() - 1);
    }

    auto ind = indices.data();
    auto leftCoeff = leftCoeffs.data();
    auto rightCoeff = rightCoeffs.data();
    auto left = buffer.left().data();
    auto right = buffer.right().data();
    while (ind < indices.end()) {
        *left = source.left()[*ind] * (*leftCoeff) + source.left()[*ind + 1] * (*rightCoeff);
        *right = source.right()[*ind] * (*leftCoeff) + source.right()[*ind + 1] * (*rightCoeff);
        left++;
        right++;
        ind++;
        leftCoeff++;
        rightCoeff++;
    }

    if (!region->shouldLoop() && (floatPosition + 1.01) > source.size()) {
        DBG("Releasing " << region->sample);
        egEnvelope.startRelease(buffer.size());
    }
}

void sfz::Voice::fillWithGenerator(StereoSpan<float> buffer)
{
    if (region->sample != "*sine")
        return;

    float step = baseFrequency * twoPi<float> / sampleRate;
    phase = ::linearRamp<float>(tempSpan1, phase, step);

    ::sin<float>(tempSpan1.first(buffer.size()), buffer.left());
    absl::c_copy(buffer.left(), buffer.right().begin());

    sourcePosition += buffer.size();
}

bool sfz::Voice::checkOffGroup(int delay [[maybe_unused]], uint32_t group) noexcept
{
    if (region != nullptr && triggerType == TriggerType::NoteOn && region->offBy && *region->offBy == group) {
        // TODO: release
        return true;
    }

    return false;
}

int sfz::Voice::getTriggerNumber() const
{
    return triggerNumber;
}

int sfz::Voice::getTriggerChannel() const
{
    return triggerNumber;
}

uint8_t sfz::Voice::getTriggerValue() const
{
    return triggerNumber;
}

sfz::Voice::TriggerType sfz::Voice::getTriggerType() const
{
    return triggerType;
}

void sfz::Voice::reset()
{
    dataReady.store(false);
    state = State::idle;
    if (region != nullptr) {
        DBG("Reset voice with sample " << region->sample);
    }
    sourcePosition = 0;
    floatPosition = 0.0f;
    region = nullptr;
    noteIsOff = false;
}

void sfz::Voice::garbageCollect()
{
    if (state == State::idle && region == nullptr)
        fileData.reset();
}
