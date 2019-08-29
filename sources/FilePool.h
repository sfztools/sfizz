#pragma once
#include "Defaults.h"
#include "StereoBuffer.h"
#include "Voice.h"
#include "readerwriterqueue.h"
#include <absl/container/flat_hash_map.h>
#include <filesystem>
#include <optional>
#include <sndfile.hh>
#include <string_view>
#include <thread>

namespace sfz {
class FilePool {
public:
    FilePool()
        : fileLoadingThread(std::thread(&FilePool::loadingThread, this))
    {
    }

    ~FilePool()
    {
        quitThread = true;
        fileLoadingThread.join();
    }
    void setRootDirectory(const std::filesystem::path& directory) noexcept { rootDirectory = directory; }
    size_t getNumPreloadedSamples() const noexcept { return preloadedData.size(); }

    struct FileInformation {
        int numChannels { 1 };
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        double sampleRate { config::defaultSampleRate };
        std::shared_ptr<StereoBuffer<float>> preloadedData;
    };
    std::optional<FileInformation> getFileInformation(std::string_view filename) noexcept;
    void enqueueLoading(Voice* voice, std::string_view sample, int numFrames) noexcept;
private:
    std::filesystem::path rootDirectory;
    struct FileLoadingInformation {
        Voice* voice;
        std::string_view sample;
        int numFrames;
    };

    moodycamel::BlockingReaderWriterQueue<FileLoadingInformation> loadingQueue;
    void loadingThread() noexcept;
    std::thread fileLoadingThread;
    bool quitThread { false };
    absl::flat_hash_map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
    LEAK_DETECTOR(FilePool);
};
}