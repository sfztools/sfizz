// Copyright (c) 2019, Paul Ferrand
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

#include "FilePool.h"
#include "AudioBuffer.h"
#include "Config.h"
#include "Debug.h"
#include "absl/types/span.h"
#include <chrono>
#include <memory>
#include <sndfile.hh>
using namespace std::chrono_literals;

template <class T>
std::unique_ptr<AudioBuffer<T>> readFromFile(SndfileHandle& sndFile, int numFrames)
{
    auto returnedBuffer = std::make_unique<AudioBuffer<T>>(sndFile.channels(), numFrames);
    if (sndFile.channels() == 1) {
        sndFile.readf(returnedBuffer->channelWriter(0), numFrames);
    } else if (sndFile.channels() == 2) {
        auto tempReadBuffer = std::make_unique<AudioBuffer<float>>(1, 2 * numFrames);
        sndFile.readf(tempReadBuffer->channelWriter(0), numFrames);
        ::readInterleaved<float>(tempReadBuffer->getSpan(0), returnedBuffer->getSpan(0), returnedBuffer->getSpan(1));
    }
    return returnedBuffer;
}

std::optional<sfz::FilePool::FileInformation> sfz::FilePool::getFileInformation(std::string_view filename) noexcept
{
    std::filesystem::path file { rootDirectory / filename };
    if (!std::filesystem::exists(file))
        return {};

    SndfileHandle sndFile(reinterpret_cast<const char*>(file.c_str()));
    if (sndFile.channels() != 1 && sndFile.channels() != 2) {
        DBG("Missing logic for " << sndFile.channels() << " channels, discarding sample " << filename);
        return {};
    }

    FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(sndFile.frames());
    returnedValue.sampleRate = static_cast<double>(sndFile.samplerate());

    SF_INSTRUMENT instrumentInfo;
    sndFile.command(SFC_GET_INSTRUMENT, &instrumentInfo, sizeof(instrumentInfo));
    if (instrumentInfo.loop_count == 1) {
        returnedValue.loopBegin = instrumentInfo.loops[0].start;
        returnedValue.loopEnd = instrumentInfo.loops[0].end;
    }

    const auto preloadedSize = [&]() {
        if (config::preloadSize == 0)
            return returnedValue.end;
        else
            return std::min(returnedValue.end, static_cast<uint32_t>(config::preloadSize));
    }();

    if (preloadedData.contains(filename)) {
        returnedValue.preloadedData = preloadedData[filename];
    } else {
        returnedValue.preloadedData = std::shared_ptr<AudioBuffer<float>>(readFromFile<float>(sndFile, preloadedSize));
        preloadedData[filename] = returnedValue.preloadedData;
    }

    return returnedValue;
}

void sfz::FilePool::enqueueLoading(Voice* voice, std::string_view sample, int numFrames) noexcept
{
    if (!loadingQueue.try_enqueue({ voice, sample, numFrames })) {
        DBG("Problem enqueuing a file read for file " << sample);
    }
}

void sfz::FilePool::loadingThread() noexcept
{
    FileLoadingInformation fileToLoad {};
    while (!quitThread) {
        if (!loadingQueue.wait_dequeue_timed(fileToLoad, 100ms))
            continue;

        if (fileToLoad.voice == nullptr) {
            DBG("Background thread error: voice is null.");
            continue;
        }

        DBG("Background loading of: " << fileToLoad.sample);
        std::filesystem::path file { rootDirectory / fileToLoad.sample };
        if (!std::filesystem::exists(file)) {
            DBG("Background thread: no file " << fileToLoad.sample << " exists.");
            continue;
        }

        SndfileHandle sndFile(reinterpret_cast<const char*>(file.c_str()));
        auto fileLoaded = std::make_unique<AudioBuffer<float>>(sndFile.channels(), fileToLoad.numFrames);
        fileToLoad.voice->setFileData(readFromFile<float>(sndFile, fileToLoad.numFrames));
    }
}