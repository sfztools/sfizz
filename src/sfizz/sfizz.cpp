// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Synth.h"
#include "Messaging.h"
#include "sfizz.hpp"
#include "absl/memory/memory.h"

sfz::Sfizz::Sfizz()
{
    synth = absl::make_unique<sfz::Synth>();
}

sfz::Sfizz::~Sfizz()
{

}

bool sfz::Sfizz::loadSfzFile(const std::string& path)
{
    return synth->loadSfzFile(path);
}

bool sfz::Sfizz::loadSfzString(const std::string& path, const std::string& text)
{
    return synth->loadSfzString(path, text);
}

bool sfz::Sfizz::loadScalaFile(const std::string& path)
{
    return synth->loadScalaFile(path);
}

bool sfz::Sfizz::loadScalaString(const std::string& text)
{
    return synth->loadScalaString(text);
}

void sfz::Sfizz::setScalaRootKey(int rootKey)
{
    return synth->setScalaRootKey(rootKey);
}

int sfz::Sfizz::getScalaRootKey() const
{
    return synth->getScalaRootKey();
}

void sfz::Sfizz::setTuningFrequency(float frequency)
{
    return synth->setTuningFrequency(frequency);
}

float sfz::Sfizz::getTuningFrequency() const
{
    return synth->getTuningFrequency();
}

void sfz::Sfizz::loadStretchTuningByRatio(float ratio)
{
    return synth->loadStretchTuningByRatio(ratio);
}

int sfz::Sfizz::getNumRegions() const noexcept
{
    return synth->getNumRegions();
}

int sfz::Sfizz::getNumGroups() const noexcept
{
    return synth->getNumGroups();
}

int sfz::Sfizz::getNumMasters() const noexcept
{
    return synth->getNumMasters();
}

int sfz::Sfizz::getNumCurves() const noexcept
{
    return synth->getNumCurves();
}

const std::vector<std::string>& sfz::Sfizz::getUnknownOpcodes() const noexcept
{
    return synth->getUnknownOpcodes();
}

size_t sfz::Sfizz::getNumPreloadedSamples() const noexcept
{
    return synth->getNumPreloadedSamples();
}

void sfz::Sfizz::setSamplesPerBlock(int samplesPerBlock) noexcept
{
    synth->setSamplesPerBlock(samplesPerBlock);
}

void sfz::Sfizz::setSampleRate(float sampleRate) noexcept
{
    synth->setSampleRate(sampleRate);
}

int sfz::Sfizz::getSampleQuality(ProcessMode mode)
{
    return synth->getSampleQuality(static_cast<sfz::Synth::ProcessMode>(mode));
}

void sfz::Sfizz::setSampleQuality(ProcessMode mode, int quality)
{
    synth->setSampleQuality(static_cast<sfz::Synth::ProcessMode>(mode), quality);
}

float sfz::Sfizz::getVolume() const noexcept
{
    return synth->getVolume();
}

void sfz::Sfizz::setVolume(float volume) noexcept
{
    synth->setVolume(volume);
}

void sfz::Sfizz::noteOn(int delay, int noteNumber, uint8_t velocity) noexcept
{
    synth->noteOn(delay, noteNumber, velocity);
}

void sfz::Sfizz::noteOff(int delay, int noteNumber, uint8_t velocity) noexcept
{
    synth->noteOff(delay, noteNumber, velocity);
}

void sfz::Sfizz::cc(int delay, int ccNumber, uint8_t ccValue) noexcept
{
    synth->cc(delay, ccNumber, ccValue);
}

void sfz::Sfizz::hdcc(int delay, int ccNumber, float normValue) noexcept
{
    synth->hdcc(delay, ccNumber, normValue);
}

void sfz::Sfizz::pitchWheel(int delay, int pitch) noexcept
{
    synth->pitchWheel(delay, pitch);
}

void sfz::Sfizz::aftertouch(int delay, uint8_t aftertouch) noexcept
{
    synth->aftertouch(delay, aftertouch);
}

void sfz::Sfizz::tempo(int delay, float secondsPerBeat) noexcept
{
    synth->tempo(delay, secondsPerBeat);
}

void sfz::Sfizz::timeSignature(int delay, int beatsPerBar, int beatUnit)
{
    synth->timeSignature(delay, beatsPerBar, beatUnit);
}

void sfz::Sfizz::timePosition(int delay, int bar, float barBeat)
{
    synth->timePosition(delay, bar, barBeat);
}

void sfz::Sfizz::playbackState(int delay, int playbackState)
{
    synth->playbackState(delay, playbackState);
}

void sfz::Sfizz::renderBlock(float** buffers, size_t numSamples, int /*numOutputs*/) noexcept
{
    synth->renderBlock({{buffers[0], buffers[1]}, numSamples});
}

int sfz::Sfizz::getNumActiveVoices() const noexcept
{
    return synth->getNumActiveVoices();
}

int sfz::Sfizz::getNumVoices() const noexcept
{
    return synth->getNumVoices();
}

void sfz::Sfizz::setNumVoices(int numVoices) noexcept
{
    synth->setNumVoices(numVoices);
}

bool sfz::Sfizz::setOversamplingFactor(int factor) noexcept
{
    using sfz::Oversampling;
    switch(factor)
    {
        case 1:
            synth->setOversamplingFactor(sfz::Oversampling::x1);
            return true;
        case 2:
            synth->setOversamplingFactor(sfz::Oversampling::x2);
            return true;
        case 4:
            synth->setOversamplingFactor(sfz::Oversampling::x4);
            return true;
        case 8:
            synth->setOversamplingFactor(sfz::Oversampling::x8);
            return true;
        default:
            return false;
    }
}


int sfz::Sfizz::getOversamplingFactor() const noexcept
{
    return static_cast<int>(synth->getOversamplingFactor());
}

void sfz::Sfizz::setPreloadSize(uint32_t preloadSize) noexcept
{
    synth->setPreloadSize(preloadSize);
}

uint32_t sfz::Sfizz::getPreloadSize() const noexcept
{
    return synth->getPreloadSize();
}

int sfz::Sfizz::getAllocatedBuffers() const noexcept
{
    return synth->getAllocatedBuffers();
}

int sfz::Sfizz::getAllocatedBytes() const noexcept
{
    return synth->getAllocatedBytes();
}

void sfz::Sfizz::enableFreeWheeling() noexcept
{
    synth->enableFreeWheeling();
}

void sfz::Sfizz::disableFreeWheeling() noexcept
{
    synth->disableFreeWheeling();
}

bool sfz::Sfizz::shouldReloadFile()
{
    return synth->shouldReloadFile();
}

bool sfz::Sfizz::shouldReloadScala()
{
    return synth->shouldReloadScala();
}

void sfz::Sfizz::enableLogging() noexcept
{
    synth->enableLogging();
}

void sfz::Sfizz::enableLogging(const std::string& prefix) noexcept
{
    synth->enableLogging(prefix);
}

void sfz::Sfizz::setLoggingPrefix(const std::string& prefix) noexcept
{
    synth->setLoggingPrefix(prefix);
}

void sfz::Sfizz::disableLogging() noexcept
{
    synth->disableLogging();
}

void sfz::Sfizz::allSoundOff() noexcept
{
    synth->allSoundOff();
}

void sfz::Sfizz::addExternalDefinition(const std::string& id, const std::string& value)
{
    synth->getParser().addExternalDefinition(id, value);
}

void sfz::Sfizz::clearExternalDefinitions()
{
    synth->getParser().clearExternalDefinitions();
}

const std::vector<std::pair<uint8_t, std::string>>& sfz::Sfizz::getKeyLabels() const noexcept
{
    return synth->getKeyLabels();
}

const std::vector<std::pair<uint16_t, std::string>>& sfz::Sfizz::getCCLabels() const noexcept
{
    return synth->getCCLabels();
}

void sfz::Sfizz::ClientDeleter::operator()(Client *client) const noexcept
{
    delete client;
}

auto sfz::Sfizz::createClient(void* data) -> ClientPtr
{
    return ClientPtr(new Client(data));
}

void* sfz::Sfizz::getClientData(Client& client)
{
    return client.getClientData();
}

void sfz::Sfizz::setReceiveCallback(Client& client, sfizz_receive_t* receive)
{
    client.setReceiveCallback(receive);
}

void sfz::Sfizz::sendMessage(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    synth->dispatchMessage(client, delay, path, sig, args);
}

void sfz::Sfizz::setBroadcastCallback(sfizz_receive_t* broadcast, void* data)
{
    synth->setBroadcastCallback(broadcast, data);
}
