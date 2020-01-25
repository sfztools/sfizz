// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Synth.h"
#include "sfizz.hpp"

sfz::Sfizz::Sfizz()
{
    synth = std::make_unique<sfz::Synth>();
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
