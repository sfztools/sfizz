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
#include "Buffer.h"
#include "AudioBuffer.h"
#include "Config.h"
#include "Debug.h"
#include "Oversampler.h"
#include "AtomicGuard.h"
#include "absl/types/span.h"
#include <chrono>
#include <memory>
#include <sndfile.hh>
#include <thread>
using namespace std::chrono_literals;

template <class T>
void readBaseFile(SndfileHandle& sndFile, sfz::AudioBuffer<T>& output, uint32_t numFrames)
{
    output.reset();
    output.resize(numFrames);
    if (sndFile.channels() == 1) {
        output.addChannel();
        sndFile.readf(output.channelWriter(0), numFrames);
    } else if (sndFile.channels() == 2) {
        output.addChannel();
        output.addChannel();
        sfz::Buffer<T> tempReadBuffer { 2 * numFrames };
        sndFile.readf(tempReadBuffer.data(), numFrames);
        sfz::readInterleaved<T>(tempReadBuffer, output.getSpan(0), output.getSpan(1));
    }
}

template <class T>
std::unique_ptr<sfz::AudioBuffer<T>> readFromFile(SndfileHandle& sndFile, uint32_t numFrames, sfz::Oversampling factor)
{
    auto baseBuffer = std::make_unique<sfz::AudioBuffer<T>>();
    readBaseFile(sndFile, *baseBuffer, numFrames);

    if (factor == sfz::Oversampling::x1)
        return baseBuffer;

    auto outputBuffer = std::make_unique<sfz::AudioBuffer<T>>(sndFile.channels(), numFrames * static_cast<int>(factor));
    sfz::Oversampler oversampler { factor };
    oversampler.stream(*baseBuffer, *outputBuffer);
    return outputBuffer;
}

template <class T>
void streamFromFile(SndfileHandle& sndFile, uint32_t numFrames, sfz::Oversampling factor, sfz::AudioBuffer<float>& output, std::atomic<size_t>* filledFrames=nullptr)
{
    if (factor == sfz::Oversampling::x1) {
        readBaseFile(sndFile, output, numFrames);
        if (filledFrames != nullptr)
            filledFrames->store(numFrames);
        return;
    }

    auto baseBuffer = readFromFile<T>(sndFile, numFrames, sfz::Oversampling::x1);
    output.reset();
    output.addChannels(baseBuffer->getNumChannels());
    output.resize(numFrames * static_cast<int>(factor));
    sfz::Oversampler oversampler { factor };
    oversampler.stream(*baseBuffer, output, filledFrames);
}

sfz::FilePool::FilePool()
{
    for (int i = 0; i < config::numBackgroundThreads; ++i)
        threadPool.emplace_back( &FilePool::loadingThread, this );

    threadPool.emplace_back( &FilePool::clearingThread, this );

    for (int i = 0; i < config::maxFilePromises; ++i)
        emptyPromises.push_back(std::make_shared<FilePromise>());
}

sfz::FilePool::~FilePool()
{
    quitThread = true;
    for (auto& thread: threadPool)
        thread.join();
}

absl::optional<sfz::FilePool::FileInformation> sfz::FilePool::getFileInformation(const std::string& filename) noexcept
{
    fs::path file { rootDirectory / filename };
    if (!fs::exists(file))
        return {};

    SndfileHandle sndFile(file.c_str());
    if (sndFile.channels() != 1 && sndFile.channels() != 2) {
        DBG("Missing logic for " << sndFile.channels() << " channels, discarding sample " << filename);
        return {};
    }

    FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(sndFile.frames());
    returnedValue.sampleRate = static_cast<double>(sndFile.samplerate());
    returnedValue.numChannels = sndFile.channels();

    SF_INSTRUMENT instrumentInfo;
    sndFile.command(SFC_GET_INSTRUMENT, &instrumentInfo, sizeof(instrumentInfo));
    if (instrumentInfo.loop_count > 0) {
        returnedValue.loopBegin = instrumentInfo.loops[0].start;
        returnedValue.loopEnd = instrumentInfo.loops[0].end;
    }

    return returnedValue;
}

bool sfz::FilePool::preloadFile(const std::string& filename, uint32_t maxOffset) noexcept
{
    fs::path file { rootDirectory / filename };
    if (!fs::exists(file))
        return false;

    SndfileHandle sndFile(file.c_str());
    if (sndFile.channels() != 1 && sndFile.channels() != 2)
        return false;

    // FIXME: Large offsets will require large preloading; is this OK in practice? Apparently sforzando does the same
    const uint32_t frames = sndFile.frames();
    const auto framesToLoad = [&]() {
        if (preloadSize == 0)
            return frames;
        else
            return min(frames, maxOffset + preloadSize);
    }();

    if (preloadedFiles.contains(filename)) {
        if (framesToLoad > preloadedFiles[filename].preloadedData->getNumFrames()) {
            preloadedFiles[filename].preloadedData = readFromFile<float>(sndFile, framesToLoad, oversamplingFactor);
        }
    } else {
        preloadedFiles.insert_or_assign(filename, {
            readFromFile<float>(sndFile, framesToLoad, oversamplingFactor),
            static_cast<float>(oversamplingFactor) * sndFile.samplerate()
        });
    }

    return true;
}

sfz::FilePromisePtr sfz::FilePool::getFilePromise(const std::string& filename) noexcept
{
    if (emptyPromises.empty()) {
        DBG("[sfizz] No empty promises left to honor the one for " << filename);
        return {};
    }

    const auto preloaded = preloadedFiles.find(filename);
    if (preloaded == preloadedFiles.end()) {
        DBG("[sfizz] File not found in the preloaded files: " << filename);
        return {};
    }

    auto promise = emptyPromises.back();
    promise->filename = preloaded->first;
    promise->preloadedData = preloaded->second.preloadedData;
    promise->sampleRate = preloaded->second.sampleRate;
    promise->oversamplingFactor = oversamplingFactor;

    if (!promiseQueue.try_push(promise)) {
        DBG("[sfizz] Could not enqueue the promise for " << filename << " (queue size " << promiseQueue.size() << ")");
        return {};
    }

    emptyPromises.pop_back();
    return promise;
}

void sfz::FilePool::setPreloadSize(uint32_t preloadSize) noexcept
{
    // Update all the preloaded sizes
    for (auto& preloadedFile : preloadedFiles) {
        const auto numFrames = preloadedFile.second.preloadedData->getNumFrames() / static_cast<int>(oversamplingFactor);
        const uint32_t maxOffset = numFrames > this->preloadSize ? numFrames - this->preloadSize : 0;
        fs::path file { rootDirectory / std::string(preloadedFile.first) };
        SndfileHandle sndFile(file.c_str());
        preloadedFile.second.preloadedData = readFromFile<float>(sndFile, preloadSize + maxOffset, oversamplingFactor);
    }
    this->preloadSize = preloadSize;
}

void sfz::FilePool::tryToClearPromises()
{
    AtomicDisabler disabler { canAddPromisesToClear };

    while (addingPromisesToClear)
        std::this_thread::sleep_for(1ms);

    for (auto& promise: promisesToClear) {
        if (promise->dataReady)
            promise->reset();
    }
}

void sfz::FilePool::clearingThread()
{
    while (!quitThread) {
        tryToClearPromises();
        std::this_thread::sleep_for(50ms);
    }
}

void sfz::FilePool::loadingThread() noexcept
{
    FilePromisePtr promise;
    while (!quitThread) {
        
        if (emptyQueue) {
            while(promiseQueue.try_pop(promise)) {
                // We're just dequeuing
            }
            emptyQueue = false;
            continue;
        }

        if (!promiseQueue.try_pop(promise)) {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        threadsLoading++;
        fs::path file { rootDirectory / std::string(promise->filename) };
        SndfileHandle sndFile(file.c_str());
        if (sndFile.error() != 0) {
            DBG("[sfizz] libsndfile errored for " << promise->filename << " with message " << sndFile.strError());
            continue;
        }
        const uint32_t frames = sndFile.frames();
        streamFromFile<float>(sndFile, frames, oversamplingFactor, promise->fileData, &promise->availableFrames);
        promise->dataReady = true;
        threadsLoading--;

        while (!filledPromiseQueue.try_push(promise)) {
            DBG("[sfizz] Error enqueuing the promise for " << promise->filename << " in the filledPromiseQueue");
            std::this_thread::sleep_for(1ms);
        }

        promise.reset();
    }
}

void sfz::FilePool::clear()
{
    emptyFileLoadingQueues();
    preloadedFiles.clear();
    temporaryFilePromises.clear();
    promisesToClear.clear();
}

void sfz::FilePool::cleanupPromises() noexcept
{
    AtomicGuard guard { addingPromisesToClear };

    if (!canAddPromisesToClear)
        return;

    // The garbage collection cleared the data from these so we can move them
    // back to the empty queue
    auto clearedIterator = promisesToClear.begin();
    auto clearedSentinel = promisesToClear.end() - 1;
    while (clearedIterator != promisesToClear.end()) {
        if (clearedIterator->get()->dataReady == false) {
            emptyPromises.push_back(*clearedIterator);
            std::iter_swap(clearedIterator, clearedSentinel);
            clearedSentinel--;
            promisesToClear.pop_back();
        } else {
            clearedIterator++;
        }
    }

    FilePromisePtr promise;
    // Remove the promises from the filled queue and put them in a linear
    // storage
    while (filledPromiseQueue.try_pop(promise))
        temporaryFilePromises.push_back(promise);

    auto filledIterator = temporaryFilePromises.begin();
    auto filledSentinel = temporaryFilePromises.end() - 1;
    while (filledIterator != temporaryFilePromises.end()) {
        if (filledIterator->use_count() == 1) {
            promisesToClear.push_back(*filledIterator);
            std::iter_swap(filledIterator, filledSentinel);
            filledSentinel--;
            temporaryFilePromises.pop_back();
        } else {
            filledIterator++;
        }
    }
}

void sfz::FilePool::setOversamplingFactor(sfz::Oversampling factor) noexcept
{
    float samplerateChange { static_cast<float>(factor) / static_cast<float>(this->oversamplingFactor) };
    for (auto& preloadedFile : preloadedFiles) {
        const auto numFrames = preloadedFile.second.preloadedData->getNumFrames() / static_cast<int>(this->oversamplingFactor);
        const uint32_t maxOffset = numFrames > this->preloadSize ? numFrames - this->preloadSize : 0;
        fs::path file { rootDirectory / std::string(preloadedFile.first) };
        SndfileHandle sndFile(file.c_str());
        preloadedFile.second.preloadedData = readFromFile<float>(sndFile, preloadSize + maxOffset, factor);
        preloadedFile.second.sampleRate *= samplerateChange;
    }

    this->oversamplingFactor = factor;
}

sfz::Oversampling sfz::FilePool::getOversamplingFactor() const noexcept
{
    return oversamplingFactor;
}

uint32_t sfz::FilePool::getPreloadSize() const noexcept
{
    return preloadSize;
}

void sfz::FilePool::emptyFileLoadingQueues() noexcept
{
    emptyQueue = true;
    while (emptyQueue)
        std::this_thread::sleep_for(1ms);
}

void sfz::FilePool::waitForBackgroundLoading() noexcept
{
    // TODO: validate that this is enough, otherwise we will need an atomic count
    // of the files we need to load still.
    // Spinlocking on the size of the background queue
    while (promiseQueue.size() > 0)
        std::this_thread::sleep_for(0.1ms);
    // Spinlocking on the threads possibly logging in the background
    while (threadsLoading > 0)
        std::this_thread::sleep_for(0.1ms);
}
