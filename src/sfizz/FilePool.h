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
#include "Voice.h"
#include "ghc/fs_std.hpp"
#include "moodycamel/readerwriterqueue.h"
#include <absl/container/flat_hash_map.h>
#include <mutex>
#include <absl/types/optional.h>
#include <string_view>
#include <thread>

namespace sfz {
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
        garbageCollectionThread.join();
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
    size_t getNumPreloadedSamples() const noexcept { return preloadedData.size(); }

    struct FileInformation {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        double sampleRate { config::defaultSampleRate };
        std::shared_ptr<AudioBuffer<float>> preloadedData;
    };
    /**
     * @brief Get metadata information about a file as well as the first chunk of data
     *
     * If the same file was already preloaded and with a compatible offset, the handle
     * is shared between the regions. Otherwise, a new handle is created (the others keep
     * the old preloaded file).
     *
     * @param filename
     * @param offset the maximum offset to consider for preloading. The total preloaded
     *                  size will be preloadedSize + offset
     * @return absl::optional<FileInformation>
     */
    absl::optional<FileInformation> getFileInformation(const std::string& filename, uint32_t offset) noexcept;
    /**
     * @brief Queue a full loading operation for a given voice.
     *
     * The goal of the ticket is to avoid file loading operations that for some reason
     * finish too late a "replace" a proper sample with an obsolete one for a voice.
     *
     * @param voice the voice to give the full file data to
     * @param sample the sample file
     * @param numFrames the number of frames to load from the file
     * @param ticket an ideally unique ticket number for this file.
     */
    void enqueueLoading(Voice* voice, const std::string* sample, int numFrames, unsigned ticket) noexcept;
    /**
     * @brief Clear all preloaded files.
     *
     */
    void clear();
    /**
     * @brief Empty the file loading queue. This function will lock and wait
     * for the background thread to finish its business, so don't call it from
     * the audio thread.
     *
     */
    void emptyFileLoadingQueue() noexcept;
private:
    fs::path rootDirectory;
    struct FileLoadingInformation {
        Voice* voice;
        const std::string* sample;
        int numFrames;
        unsigned ticket;
    };

    moodycamel::BlockingReaderWriterQueue<FileLoadingInformation> loadingQueue { config::numVoices };
    void loadingThread() noexcept;
    void garbageThread() noexcept;

    Oversampling oversamplingFactor { config::defaultOversamplingFactor };

    // Signals
    bool quitThread { false };
    bool emptyQueue { false };

    std::mutex fileHandleMutex;
    std::vector<std::shared_ptr<AudioBuffer<float>>> fileHandles;
    absl::flat_hash_map<absl::string_view, std::shared_ptr<AudioBuffer<float>>> preloadedData;
    std::thread fileLoadingThread { &FilePool::loadingThread, this };
    std::thread garbageCollectionThread { &FilePool::garbageThread, this };
    LEAK_DETECTOR(FilePool);
};
}
