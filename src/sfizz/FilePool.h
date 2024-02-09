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
#include "RTSemaphore.h"
#include "AudioBuffer.h"
#include "AudioSpan.h"
#include "FileId.h"
#include "FileMetadata.h"
#include "SIMDHelpers.h"
#include "SpinMutex.h"
#include "utility/Timing.h"
#include "utility/LeakDetector.h"
#include "utility/MemoryHelpers.h"
#include <ghc/fs_std.hpp>
#include <absl/container/flat_hash_map.h>
#include <absl/types/optional.h>
#include <absl/strings/string_view.h>
#include <atomic_queue/atomic_queue.h>
#include <chrono>
#include <thread>
#include <future>
#include <memory>
class ThreadPool;

namespace sfz {
class FilePool;

using FileAudioBuffer = AudioBuffer<float, 2, config::defaultAlignment,
                                    sfz::config::excessFileFrames, sfz::config::excessFileFrames>;
using FileAudioBufferPtr = std::shared_ptr<FileAudioBuffer>;

struct FileInformation {
    int64_t end { Default::sampleEnd };
    int64_t maxOffset { 0 };
    int64_t loopStart { Default::loopStart };
    int64_t loopEnd { Default::loopEnd };
    bool hasLoop { false };
    double sampleRate { config::defaultSampleRate };
    int numChannels { 0 };
    int rootKey { 0 };
    absl::optional<WavetableInfo> wavetable;
};

// Strict C++11 disallows member initialization if aggregate initialization is to be used...
struct FileData
{
    enum class Status { Invalid, Preloaded, Streaming, Done, FullLoaded };
    FileData()
    {
    }
    FileData(FileAudioBuffer preloaded, FileInformation info)
    : preloadedData(std::move(preloaded)), information(std::move(info))
    {

    }

    struct AudioSpanData {
        AudioSpanData(AudioSpanData&& other)
        : data(other.data),
          fileData(other.fileData)
        {}

        AudioSpan<const float> data;

        friend struct FileData;
    private:
        AudioSpanData(AudioSpan<const float>&& data, FileData& fileData, std::unique_lock<SpinMutex>&& lock)
        : data(data),
          fileData(fileData)
        {
            std::unique_lock<SpinMutex> l = std::move(lock);
            fileData.dataReaderCount++;
        }

    public:
        ~AudioSpanData() {
            std::unique_lock<SpinMutex> l { fileData.garbageMutex };
            fileData.lastViewerLeftAt = highResNow();
            fileData.dataReaderCount--;
        }

    private:
        FileData& fileData;
    };

    AudioSpanData getData()
    {
        std::unique_lock<SpinMutex> lock { garbageMutex };
        if (availableFrames > preloadedData.getNumFrames())
            return { AudioSpan<const float>(fileData).first(availableFrames), *this, std::move(lock) };
        else
            return { AudioSpan<const float>(preloadedData), *this, std::move(lock) };
    };

    FileData(const FileData& other) = delete;
    FileData& operator=(const FileData& other) = delete;
    FileData(FileData&& other) = delete;
    FileData& operator=(FileData&& other) = delete;

    void initWith(Status newStatus, FileData&& other) {
        ASSERT(other.readerCount == 0); // Probably should not be moving this...
        ASSERT(other.preloadCallCount == 0);
        information = std::move(other.information);
        preloadedData = std::move(other.preloadedData);
        fileData = std::move(other.fileData);
        availableFrames = other.availableFrames.load();
        lastViewerLeftAt = other.lastViewerLeftAt;
        status = newStatus;
        {
            std::lock_guard<std::mutex> guard { readyMutex };
            ready = true;
        }
        readyCond.notify_all();
    }

    void addOwner(FilePool* owner);
    void prepareForRemovingOwner(FilePool* owner);
    bool checkAndRemoveOwner(FilePool* owner, const FileId &fileId);
    bool addSecondaryOwner(FilePool* owner);
    bool canRemove() const { return preloadCallCount == 0 && ready; };
    bool isReady() { return ready; }

    FileAudioBuffer preloadedData;
    FileInformation information;
    FileAudioBuffer fileData {};

private:
    bool ready { false };
    std::condition_variable readyCond;
    std::mutex readyMutex;
    std::mutex ownerMutex;
    int preloadCallCount { 0 };
    std::map<FilePool*, bool> ownerMap;

public:
    SpinMutex garbageMutex;
    std::atomic<Status> status { Status::Invalid };
    std::atomic<size_t> availableFrames { 0 };
    std::atomic<uint32_t> readerCount { 0 };
    uint32_t dataReaderCount { 0 };
    std::chrono::time_point<std::chrono::high_resolution_clock> lastViewerLeftAt;

    LEAK_DETECTOR(FileData);
};


class FileDataHolder {
public:
    FileDataHolder() = default;
    FileDataHolder(const FileDataHolder&) = delete;
    FileDataHolder& operator=(const FileDataHolder&) = delete;
    FileDataHolder(FileDataHolder&& other)
    {
        this->data = other.data;
        other.data = nullptr;
    }
    FileDataHolder& operator=(FileDataHolder&& other)
    {
        this->data = other.data;
        other.data = nullptr;
        return *this;
    }
    FileDataHolder(const std::shared_ptr<FileData>& data) : data(data)
    {
        if (!data)
            return;

        data->readerCount += 1;
    }
    void reset()
    {
        if (!data)
            return;

        data->readerCount -= 1;
        data->lastViewerLeftAt = highResNow();
        data = nullptr;
    }
    ~FileDataHolder()
    {
        reset();
    }
    FileData& operator*() { return *data; }
    FileData* operator->() { return data.get(); }
    explicit operator bool() const { return (bool)data; }
private:
    std::shared_ptr<FileData> data { nullptr };
    LEAK_DETECTOR(FileDataHolder);
};

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
    FilePool(bool& freeWheel);

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
    size_t getNumPreloadedSamples() const noexcept { return preloadedFiles.size() + loadedFiles.size(); }

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
    FileDataHolder loadFile(const FileId& fileId) noexcept;

    /**
     * @brief Load a file from RAM and return its information. The file pool will store\
     * this data for future requests so use this function responsibly.
     *
     * @param fileId
     * @param data
     * @return A handle on the file data
     */
    FileDataHolder loadFromRam(const FileId& fileId, const std::vector<char>& data) noexcept;

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
     * @brief Reset the number of preloadFile counts for each sample.
     */
    void resetPreloadCallCounts() noexcept;

    /**
     * @brief Clear all files with a preload call count of 0.
     */
    void removeUnusedPreloadedData() noexcept;

    /**
     * @brief Get a handle on a file, which triggers background loading
     *
     * @param fileId the file to preload
     * @return FileDataHolder a file data handle
     */
    FileDataHolder getFilePromise(const std::shared_ptr<FileId>& fileId) noexcept;
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
     * @brief Empty the file loading queues without actually loading
     * the files. All promises will be unfulfilled. Don't call this
     * method on the audio thread as it will spinlock.
     *
     */
    void emptyFileLoadingQueues() noexcept
    {
        // nothing to do in this implementation,
        // deleting the region and its sample ID take care of it
    }
    /**
     * @brief Wait for the background loading to finish for all promises
     * in the queue.
     */
    void waitForBackgroundLoading() noexcept;
    /**
     * @brief Assign the current thread a priority which is appropriate
     * for background sample file processing.
     */
    static void raiseCurrentThreadPriority() noexcept;
    /**
     * @brief Change whether all samples are loaded in ram.
     * This will trigger a purge and reloading.
     *
     * @param loadInRam
     */
    void setRamLoading(bool loadInRam) noexcept;

    /**
     * @brief Notify when render starts 
     */
    void startRender();

    /**
     * @brief Notify when render finishes
     */
    void stopRender();

    /**
     * @brief Allocate this when rendering
     */
    class RenderLock
    {
        public:
        RenderLock(FilePool& filePool)
        : filePool(filePool)
        {
            filePool.startRender();
        }
        ~RenderLock()
        {
            filePool.stopRender();
        }

        private:
        FilePool& filePool;
    };
private:

    absl::optional<sfz::FileInformation> checkExistingFileInformation(const FileId& fileId) noexcept;
    fs::path rootDirectory;

    bool loadInRam { config::loadInRam };
    uint32_t preloadSize { config::preloadSize };

    // Signals
    volatile bool dispatchFlag { true };
    RTSemaphore dispatchBarrier { 0 };

    // Structures for the background loaders
    struct QueuedFileData
    {
        QueuedFileData() noexcept {}
        QueuedFileData(std::weak_ptr<FileId> id, const std::shared_ptr<FileData>& data) noexcept
        : id(id), data(data) {}
        std::weak_ptr<FileId> id;
        std::shared_ptr<FileData> data { nullptr };
    };

    using FileQueue = atomic_queue::AtomicQueue2<QueuedFileData, config::maxVoices>;
    aligned_unique_ptr<FileQueue> filesToLoad;

    void dispatchingJob() noexcept;
    void loadingJob(const QueuedFileData& data) noexcept;
    std::mutex loadingJobsMutex;
    std::vector<std::future<void>> loadingJobs;
    std::thread dispatchThread { &FilePool::dispatchingJob, this };

public:
    struct GlobalObject
    {
        friend class sfz::FilePool;
        friend struct sfz::FileData;
        GlobalObject(size_t num_threads);

        public:
        ~GlobalObject();

        public:
        size_t getNumLoadedSamples() {
            return loadedFiles.size() + preloadedFiles.size();
        }

        private:
        std::thread garbageThread { &GlobalObject::garbageJob, this };
        volatile bool garbageFlag { true };
        static RTSemaphore semGarbageBarrier;
        std::atomic<uint32_t> runningRender = { 0 };
        void garbageJob();

        std::unique_ptr<ThreadPool> threadPool;

        std::mutex loadedFilesMutex;
        std::mutex preloadedFilesMutex;

        // Preloaded data
        absl::flat_hash_map<FileId, std::shared_ptr<FileData>> preloadedFiles;
        absl::flat_hash_map<FileId, std::shared_ptr<FileData>> loadedFiles;
    };
    static std::shared_ptr<GlobalObject> getGlobalObject();

private:
    std::shared_ptr<GlobalObject> globalObject;
    static std::weak_ptr<GlobalObject> globalObjectWeakPtr;
    static std::mutex globalObjectMutex;

    // Preloaded data
    absl::flat_hash_map<FileId, std::shared_ptr<FileData>> preloadedFiles;
    absl::flat_hash_map<FileId, std::shared_ptr<FileData>> loadedFiles;

    bool& freeWheeling;

    LEAK_DETECTOR(FilePool);
};
}
