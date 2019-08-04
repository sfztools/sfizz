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
    void setRootDirectory(const std::filesystem::path& directory)
    {
        rootDirectory = directory;
    }

    struct FileInformation
    {
        uint32_t end { Default::sampleEndRange.getEnd() };
        uint32_t loopBegin { Default::loopRange.getStart() };
        uint32_t loopEnd { Default::loopRange.getEnd() };
        std::shared_ptr<StereoBuffer<float>> preloadedData;
    };

    std::optional<FileInformation> getFileInformation(std::string_view filename)
    {
        std::filesystem::path file { rootDirectory / filename };
        if (!std::filesystem::exists(file))
            return {};
        
        SndfileHandle sndFile ( reinterpret_cast<const char*>(file.c_str()) );
        FileInformation returnedValue;
        returnedValue.end = static_cast<uint32_t>(sndFile.frames());
        SF_INSTRUMENT instrumentInfo;
        sndFile.command(SFC_GET_INSTRUMENT, &instrumentInfo, sizeof(instrumentInfo));
        if (instrumentInfo.loop_count == 1)
        {
            returnedValue.loopBegin = instrumentInfo.loops[0].start;
            returnedValue.loopEnd = instrumentInfo.loops[0].end;
        }
        auto preloadedSize = std::min(returnedValue.end, static_cast<uint32_t>(config::preloadSize));
        returnedValue.preloadedData = std::make_shared<StereoBuffer<float>>(preloadedSize);
        sndFile.readf(tempReadBuffer.data(), preloadedSize);
        returnedValue.preloadedData->readInterleaved<SIMDConfig::supported>(tempReadBuffer.data(), preloadedSize);
        preloadedData[filename] = returnedValue.preloadedData;
        // char  buffer [2048] ;
        // sndFile.command(SFC_GET_LOG_INFO, buffer, sizeof(buffer)) ;
        // DBG(buffer);
        return returnedValue;
    }
    size_t getNumPreloadedSamples() { return preloadedData.size(); }
private:
    std::filesystem::path rootDirectory;
    Buffer<float> tempReadBuffer { config::preloadSize * 2 };
    // std::map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
    absl::flat_hash_map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
};

static inline FilePool filePool;
}