#include "MidiState.h"
#include "Debug.h"

sfz::MidiState::MidiState()
{
    reset();
}

void sfz::MidiState::noteOnEvent(int channel, int noteNumber, uint8_t velocity) noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    ASSERT(velocity >= 0 && velocity <= 127);
    channel = translateSfzChannelToMidi(channel);
    if (noteNumber >= 0 && noteNumber < 128) {
        lastNoteVelocities[channel][noteNumber] = velocity;
        noteOnTimes[channel][noteNumber] = std::chrono::steady_clock::now();
    }
}

float sfz::MidiState::getNoteDuration(int channel, int noteNumber) const
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    channel = translateSfzChannelToMidi(channel);
    if (noteNumber >= 0 && noteNumber < 128) {
        const auto noteOffTime = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(noteOffTime - noteOnTimes[channel][noteNumber]);
        return duration.count();
    }

    return 0.0f;
}

uint8_t sfz::MidiState::getNoteVelocity(int channel, int noteNumber) const noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(noteNumber >= 0 && noteNumber <= 127);
    channel = translateSfzChannelToMidi(channel);
    return lastNoteVelocities[channel][noteNumber];
}

void sfz::MidiState::pitchBendEvent(int channel, int pitchBendValue) noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(pitchBendValue >= -8192 && pitchBendValue <= 8192);
    channel = translateSfzChannelToMidi(channel);
    pitchBends[channel] = pitchBendValue;
}

int sfz::MidiState::getPitchBend(int channel) const noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    channel = translateSfzChannelToMidi(channel);
    return pitchBends[channel];
}

void sfz::MidiState::ccEvent(int channel, int ccNumber, uint8_t ccValue) noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(ccNumber >= 0 && ccNumber <= 127);
    ASSERT(ccValue >= 0 && ccValue <= 127);
    channel = translateSfzChannelToMidi(channel);
    cc[channel][ccNumber] = ccValue;
}

uint8_t sfz::MidiState::getCCValue(int channel, int ccNumber) const noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    ASSERT(ccNumber >= 0 && ccNumber <= 127);
    channel = translateSfzChannelToMidi(channel);
    return cc[channel][ccNumber];
}

const sfz::CCValueArray& sfz::MidiState::getCCArray(int channel) const noexcept
{
    ASSERT(channel >= 1 && channel <= 16);
    channel = translateSfzChannelToMidi(channel);
    return cc[channel];
}

void sfz::MidiState::reset() noexcept
{
    for (auto& channelArray: lastNoteVelocities)
        for (auto& velocity: channelArray)
            velocity = 0;

    for (auto& channelArray: cc)
        for (auto& ccValue: channelArray)
            ccValue = 0;

    for (auto& bendValue: pitchBends)
        bendValue = 0;
}