#include "FilePool.h"
#include <chrono>
using namespace std::chrono_literals;

std::optional<sfz::FilePool::FileInformation> sfz::FilePool::getFileInformation(std::string_view filename)
{
    std::filesystem::path file { rootDirectory / filename };
    if (!std::filesystem::exists(file))
        return {};
    
    SndfileHandle sndFile ( reinterpret_cast<const char*>(file.c_str()) );
    FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(sndFile.frames());
    returnedValue.sampleRate = static_cast<double>(sndFile.samplerate());
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

void sfz::FilePool::enqueueLoading(Voice* voice, std::string_view sample, int numFrames)
{
    if (!loadingQueue.try_enqueue({ voice, sample, numFrames }))
    {
        DBG("Problem enqueuing a file read for file " << sample);
    }
}

void sfz::FilePool::loadingThread()
{
    FileLoadingInformation fileToLoad {};
    while (!quitThread)
    {
        if (!loadingQueue.wait_dequeue_timed(fileToLoad, 1ms))
            continue;

        if (fileToLoad.voice == nullptr)
        {
            DBG("Background thread error: voice is null.");
            continue;
        }

        DBG("Background loading of: " << fileToLoad.sample);
        std::filesystem::path file { rootDirectory / fileToLoad.sample };
        if (!std::filesystem::exists(file))
        {
            DBG("Background thread: no file " << fileToLoad.sample << " exists.");
            continue;
        }
        
        SndfileHandle sndFile ( reinterpret_cast<const char*>(file.c_str()) );
        auto fileLoaded = std::make_unique<StereoBuffer<float>>(fileToLoad.numFrames);
        auto readBuffer = std::make_unique<Buffer<float>>(fileToLoad.numFrames * 2);
        sndFile.readf(readBuffer->data(), fileToLoad.numFrames);
        fileLoaded->readInterleaved<SIMDConfig::useSIMD>(readBuffer->data(), fileToLoad.numFrames);
        fileToLoad.voice->setFileData(std::move(fileLoaded));
    }

}