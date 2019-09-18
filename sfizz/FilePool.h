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
#include "filesystem.h"
#include "readerwriterqueue.h"
#include <absl/container/flat_hash_map.h>
#include <mutex>
#include <absl/types/optional.h>
#include <string_view>
#include <thread>

namespace sfz {
class FilePool {
public:
    FilePool()
        : fileLoadingThread(std::thread(&FilePool::loadingThread, this))
        , garbageCollectionThread(std::thread(&FilePool::garbageThread, this))
    {
    }

    ~FilePool()
    {
        quitThread = true;
        fileLoadingThread.join();
        garbageCollectionThread.join();
    }
    void setRootDirectory(const std::filesystem::path& directory) noexcept { rootDirectory = directory; }
    size_t getNumPreloadedSamples() const noexcept { return preloadedData.size(); }

    struct FileInformation {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        double sampleRate { config::defaultSampleRate };
        std::shared_ptr<AudioBuffer<float>> preloadedData;
    };
    absl::optional<FileInformation> getFileInformation(absl::string_view filename, uint32_t offset) noexcept;
    void enqueueLoading(Voice* voice, absl::string_view sample, int numFrames, unsigned ticket) noexcept;
    void clear();
private:
    std::filesystem::path rootDirectory;
    struct FileLoadingInformation {
        Voice* voice;
        absl::string_view sample;
        int numFrames;
        unsigned ticket;
    };

    moodycamel::BlockingReaderWriterQueue<FileLoadingInformation> loadingQueue { config::numVoices };
    void loadingThread() noexcept;
    void garbageThread() noexcept;
    std::thread fileLoadingThread;
    std::thread garbageCollectionThread;
    std::vector<std::shared_ptr<AudioBuffer<float>>> fileHandles;
    std::mutex fileHandleMutex;
    bool quitThread { false };
    absl::flat_hash_map<absl::string_view, std::shared_ptr<AudioBuffer<float>>> preloadedData;
    LEAK_DETECTOR(FilePool);
};
}
