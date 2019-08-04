#pragma once
#include "StereoBuffer.h"
#include "Defaults.h"
#include <sndfile.hh>
#include <filesystem>
#include <optional>
#include <string_view>
#include <absl/container/flat_hash_map.h>
#include <map>

namespace sfz
{
class FilePool
{
public:
    FilePool() = default;
    void setRootDirectory(const std::filesystem::path& directory) { rootDirectory = directory; }
    size_t getNumPreloadedSamples() { return preloadedData.size(); }

    struct FileInformation
    {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        std::shared_ptr<StereoBuffer<float>> preloadedData;
    };
    std::optional<FileInformation> getFileInformation(std::string_view filename);
private:
    std::filesystem::path rootDirectory;
    Buffer<float> tempReadBuffer { config::preloadSize * 2 };
    // std::map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
    absl::flat_hash_map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
};
}