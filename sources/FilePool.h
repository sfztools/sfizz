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
    void setRootDirectory(const std::filesystem::path& directory) { rootDirectory = directory; }
    size_t getNumPreloadedSamples() { return preloadedData.size(); }

    struct FileInformation {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        double sampleRate { config::defaultSampleRate };
        std::shared_ptr<StereoBuffer<float>> preloadedData;
    };
    std::optional<FileInformation> getFileInformation(std::string_view filename);
    void enqueueLoading(Voice* voice, std::string_view sample, int numFrames);
    static void deleteAndTrackBuffers(StereoBuffer<float>* buffer)
    {
        fileBuffers--;
        delete buffer;
    };
    static int getFileBuffers()
    {
        return fileBuffers.load();
    }

private:
    std::filesystem::path rootDirectory;
    struct FileLoadingInformation {
        Voice* voice;
        std::string_view sample;
        int numFrames;
    };

    inline static std::atomic<int> fileBuffers { 0 };
    moodycamel::BlockingReaderWriterQueue<FileLoadingInformation> loadingQueue;
    void loadingThread();
    std::thread fileLoadingThread;
    bool quitThread { false };
    absl::flat_hash_map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
    LEAK_DETECTOR(FilePool);
};
}