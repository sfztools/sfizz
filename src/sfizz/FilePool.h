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

#pragma once
#include "Config.h"
#include "Defaults.h"
#include "LeakDetector.h"
#include "AudioBuffer.h"
#include "SIMDHelpers.h"
#include "ghc/fs_std.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/types/optional.h>
#include "absl/strings/string_view.h"
#include "moodycamel/readerwriterqueue.h"
#include <thread>
#include <sndfile.hh>

namespace sfz {
using AudioBufferPtr = std::shared_ptr<AudioBuffer<float>>;


struct PreloadedFileHandle
{
    std::shared_ptr<AudioBuffer<float>> preloadedData {};
    float sampleRate { config::defaultSampleRate };
};

struct FilePromise
{
    absl::string_view filename {};
    AudioBufferPtr preloadedData {};
    std::unique_ptr<AudioBuffer<float>> fileData {};
    float sampleRate { config::defaultSampleRate };
    std::atomic<bool> dataReady { false };
    Oversampling oversamplingFactor { config::defaultOversamplingFactor };
};

using FilePromisePtr = std::shared_ptr<FilePromise>;
/**
 * @brief This is a singleton-designed class that holds all the preloaded
 * data as well as functions to request new file data and collect the file
 * handles to close after they are read.
 *
 * This object caches the file data that was already preloaded in case it is asked
 * again by a region using the same sample. In this situation, both regions have a
 * handle on the same preloaded data.
 *
 * The file request is immediately served using the preloaded data. A ticket is then
 * provided to the voice that requested the file, and the file loading happens in the
 * background. When the file is fully loaded, the background makes the full data available
 * to the voice and consumes the ticket, while conserving a handle on this file. When the
 * voice dies it releases its handle on the files, which should decrease the  reference count
 * to 1. A garbage collection thread then runs regularly to clear the memory of all file
 * handles with a reference count of 1.
 */


class FilePool {
public:
    FilePool() { }

    ~FilePool()
    {
        quitThread = true;
        fileLoadingThread.join();
    }
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

    struct FileInformation {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        double sampleRate { config::defaultSampleRate };
        int numChannels { 0 };
    };

    /**
     * @brief Get metadata information about a file.
     *
     * @param filename
     * @return absl::optional<FileInformation>
     */
    absl::optional<FileInformation> getFileInformation(const std::string& filename) noexcept;

    /**
     * @brief Check that a file is preloaded with the proper offset bounds
     *
     * @param filename
     * @param offset the maximum offset to consider for preloading. The total preloaded
     *                  size will be preloadSize + offset
     * @return true if the preloading went fine
     * @return false if something went wrong ()
     */
    bool preloadFile(const std::string& filename, uint32_t maxOffset) noexcept;

    /**
     * @brief Clear all preloaded files.
     *
     */
    void clear();

    void cleanupPromises() noexcept;
    FilePromisePtr getFilePromise(const std::string& filename) noexcept;
    void setPreloadSize(uint32_t preloadSize) noexcept;
    uint32_t getPreloadSize() const noexcept;
    void setOversamplingFactor(Oversampling factor) noexcept;
    Oversampling getOversamplingFactor() const noexcept;
    void emptyFileLoadingQueue() noexcept;
private:
    fs::path rootDirectory;
    void loadingThread() noexcept;

    moodycamel::ReaderWriterQueue<FilePromisePtr> promiseQueue;
    uint32_t preloadSize { config::preloadSize };
    Oversampling oversamplingFactor { config::defaultOversamplingFactor };
    // Signals
    bool quitThread { false };
    bool emptyQueue { false };

    std::vector<FilePromisePtr> temporaryFilePromises;
    std::vector<FilePromisePtr> promisesToClean;
    absl::flat_hash_map<absl::string_view, PreloadedFileHandle> preloadedFiles;
    std::thread fileLoadingThread { &FilePool::loadingThread, this };
    LEAK_DETECTOR(FilePool);
};
}
