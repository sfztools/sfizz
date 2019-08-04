#include "FilePool.h"

std::optional<sfz::FilePool::FileInformation> sfz::FilePool::getFileInformation(std::string_view filename)
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
    returnedValue.preloadedData->readInterleaved<SIMDConfig::useSIMD>(tempReadBuffer.data(), preloadedSize);
    preloadedData[filename] = returnedValue.preloadedData;
    // char  buffer [2048] ;
    // sndFile.command(SFC_GET_LOG_INFO, buffer, sizeof(buffer)) ;
    // DBG(buffer);
    return returnedValue;
}