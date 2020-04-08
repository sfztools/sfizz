// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Synth.h"
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

void sfz::Sfizz::pitchWheel(int delay, int pitch) noexcept
{
    synth->pitchWheel(delay, pitch);
}

void sfz::Sfizz::aftertouch(int delay, uint8_t aftertouch) noexcept
{
    synth->aftertouch(delay, aftertouch);
}

void sfz::Sfizz::tempo(int delay, float secondsPerQuarter) noexcept
{
    synth->tempo(delay, secondsPerQuarter);
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

const std::vector<std::pair<uint8_t, std::string>>& sfz::Sfizz::getNoteLabels() const noexcept
{
    return synth->getNoteLabels();
}

const std::vector<std::pair<uint16_t, std::string>>& sfz::Sfizz::getCCLabels() const noexcept
{
    return synth->getCCLabels();
}
