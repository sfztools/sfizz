// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
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

#pragma once
#include "Config.h"
#include "Defaults.h"
#include "LeakDetector.h"
#include "RTSemaphore.h"
#include "AudioBuffer.h"
#include "AudioSpan.h"
#include "FileId.h"
#include "SIMDHelpers.h"
#include "ghc/fs_std.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/types/optional.h>
#include "absl/strings/string_view.h"
#include "atomic_queue/atomic_queue.h"
#include "Logger.h"
#include <chrono>
#include <thread>
#include <mutex>

namespace sfz {
using AudioBufferPtr = std::shared_ptr<AudioBuffer<float>>;

struct FileInformation {
    uint32_t end { Default::sampleEndRange.getEnd() };
    uint32_t loopBegin { Default::loopRange.getStart() };
    uint32_t loopEnd { Default::loopRange.getEnd() };
    double sampleRate { config::defaultSampleRate };
    int numChannels { 0 };
};

// Strict C++11 disallows member initialization if aggregate initialization is to be used...
struct FileDataHandle
{
    std::shared_ptr<AudioBuffer<float>> preloadedData;
    FileInformation information;
};

struct FilePromise
{
    AudioSpan<const float> getData()
    {
        if (dataStatus == DataStatus::Ready)
            return AudioSpan<const float>(fileData)
                .first(fileData.getNumFrames() - sfz::config::excessFileFrames);
        else if (availableFrames > preloadedData->getNumFrames())
            return AudioSpan<const float>(fileData).first(availableFrames);
        else
            return AudioSpan<const float>(*preloadedData);
    }

    void reset()
    {
        fileData.reset();
        preloadedData.reset();
        fileId = FileId {};
        availableFrames = 0;
        dataStatus = DataStatus::Wait;
        oversamplingFactor = config::defaultOversamplingFactor;
        sampleRate = config::defaultSampleRate;
    }

    void waitCompletion()
    {
        while (dataStatus == DataStatus::Wait)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    enum class DataStatus {
        Wait = 0,
        Ready,
        Error,
    };

    FileId fileId {};
    AudioBufferPtr preloadedData {};
    AudioBuffer<float> fileData {};
    float sampleRate { config::defaultSampleRate };
    Oversampling oversamplingFactor { config::defaultOversamplingFactor };
    std::atomic<size_t> availableFrames { 0 };
    std::atomic<DataStatus> dataStatus { DataStatus::Wait };
    std::chrono::time_point<std::chrono::high_resolution_clock> creationTime;

    LEAK_DETECTOR(FilePromise);
};

using FilePromisePtr = std::shared_ptr<FilePromise>;
/**
 * @brief This is a singleton-designed class that holds all the preloaded data
 * as well as functions to request new file data and collect the file handles to
 * close after they are read.
 *
 * This object caches the file data that was already preloaded in case it is
 * asked again by a region using the same sample. In this situation, both
 * regions have a handle on the same preloaded data.
 *
 * The file request is immediately served using the preloaded data. A promise is
 * then provided to the voice that requested the file, and the file loading
 * happens in the background. File reads happen on whole samples but
 * oversampling is done in chunks, and the promise contains a counter for the
 * frames that are loaded. When the voice dies it releases its handle on the
 * promise, which should decrease the  reference count to 1. A garbage
 * collection thread then runs regularly to clear the memory of all file handles
 * with a reference count of 1.
 */


class FilePool {
public:
    /**
     * @brief Construct a new File Pool object.
     *
     * This creates the background threads based on config::numBackgroundThreads
     * as well as the garbage collection thread.
     */
    FilePool(Logger& logger);

    ~FilePool();
    /**
     * @brief Set the root directory from which to search for files to load
     *
     * @param directory
     */
    void setRootDirectory(const fs::path& directory) noexcept { rootDirectory = directory; }
    /**
     * @brief Get the number of preloaded sample files
     *
     * @return size_t
     */
    size_t getNumPreloadedSamples() const noexcept { return preloadedFiles.size(); }

    /**
     * @brief Get metadata information about a file.
     *
     * @param fileId
     * @return absl::optional<FileInformation>
     */
    absl::optional<FileInformation> getFileInformation(const FileId& fileId) noexcept;

    /**
     * @brief Preload a file with the proper offset bounds
     *
     * @param fileId
     * @param maxOffset the maximum offset to consider for preloading. The total preloaded
     *                  size will be preloadSize + offset
     * @return true if the preloading went fine
     * @return false if something went wrong ()
     */
    bool preloadFile(const FileId& fileId, uint32_t maxOffset) noexcept;

    /**
     * @brief Load a file and return its information. The file pool will store this
     * data for future requests so use this function responsibly.
     *
     * @param fileId
     * @return A handle on the file data
     */
    absl::optional<sfz::FileDataHandle> loadFile(const FileId& fileId) noexcept;

    /**
     * @brief Check that the sample exists. If not, try to find it in a case insensitive way.
     *
     * @param filename the sample filename; may be updated by the method
     * @return true if the sample exists or was updated properly
     * @return false if no sample was found even with a case insensitive search
     */
    bool checkSample(std::string& filename) const noexcept;

    /**
     * @brief Check that the sample exists. If not, try to find it in a case insensitive way.
     *
     * @param fileId the sample file identifier; may be updated by the method
     * @return true if the sample exists or was updated properly
     * @return false if no sample was found even with a case insensitive search
     */
    bool checkSampleId(FileId& fileId) const noexcept;

    /**
     * @brief Clear all preloaded files.
     *
     */
    void clear();
    /**
     * @brief Moves the filled promises to a linear storage, and checks
     * said linear storage for promises that are not used anymore.
     *
     * This function has to be called on the audio thread.
     */
    void cleanupPromises() noexcept;
    /**
     * @brief Get a file promise
     *
     * @param fileId the file to preload
     * @return FilePromisePtr a file promise
     */
    FilePromisePtr getFilePromise(const FileId& fileId) noexcept;
    /**
     * @brief Change the preloading size. This will trigger a full
     * reload of all samples, so don't call it on the audio thread.
     *
     * @param preloadSize
     */
    void setPreloadSize(uint32_t preloadSize) noexcept;
    /**
     * @brief Get the current preload size.
     *
     * @return uint32_t
     */
    uint32_t getPreloadSize() const noexcept;
    /**
     * @brief Set the oversampling factor. This will trigger a full
     * reload of all samples so don't call it on the audio thread.
     *
     * @param factor
     */
    void setOversamplingFactor(Oversampling factor) noexcept;
    /**
     * @brief Get the current oversampling factor
     *
     * @return Oversampling
     */
    Oversampling getOversamplingFactor() const noexcept;
    /**
     * @brief Empty the file loading queues without actually loading
     * the files. All promises will be unfulfilled. Don't call this
     * method on the audio thread as it will spinlock.
     *
     */
    void emptyFileLoadingQueues() noexcept;
    /**
     * @brief Wait for the background loading to finish for all promises
     * in the queue.
     */
    void waitForBackgroundLoading() noexcept;
private:
    Logger& logger;
    fs::path rootDirectory;
    void loadingThread() noexcept;
    void clearingThread();
    void tryToClearPromises();

    atomic_queue::AtomicQueue2<FilePromisePtr, config::maxVoices> promiseQueue;
    atomic_queue::AtomicQueue2<FilePromisePtr, config::maxVoices> filledPromiseQueue;
    uint32_t preloadSize { config::preloadSize };
    Oversampling oversamplingFactor { config::defaultOversamplingFactor };
    // Signals
    volatile bool quitThread { false };
    volatile bool emptyQueue { false };
    std::atomic<int> threadsLoading { 0 };
    RTSemaphore workerBarrier;

    // File promises data structures along with their guards.
    std::vector<FilePromisePtr> emptyPromises;
    std::vector<FilePromisePtr> temporaryFilePromises;
    std::vector<FilePromisePtr> promisesToClear;
    std::mutex promiseGuard;

    // Preloaded data
    absl::flat_hash_map<FileId, FileDataHandle> preloadedFiles;
    absl::flat_hash_map<FileId, FileDataHandle> loadedFiles;
    std::vector<std::thread> threadPool { };
    LEAK_DETECTOR(FilePool);
};
}
