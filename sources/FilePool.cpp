#include "FilePool.h"
#include "Globals.h"
#include "absl/types/span.h"
#include <chrono>
#include <fstream>
#include <memory>
#include <sndfile.hh>
#include <sstream>
#include <string>
using namespace std::chrono_literals;


template<class T>
void readFromFile(SndfileHandle& sndFile, int numFrames, StereoBuffer<T>& output)
{
    if (sndFile.channels() == 1) {
        auto tempReadBuffer = std::make_unique<Buffer<float>>(numFrames);
        sndFile.readf(tempReadBuffer->data(), numFrames);
        std::copy(tempReadBuffer->begin(), tempReadBuffer->end(), output.begin(Channel::left));
        std::copy(tempReadBuffer->begin(), tempReadBuffer->end(), output.begin(Channel::right));
    } else if (sndFile.channels() == 2) {
        auto tempReadBuffer = std::make_unique<Buffer<float>>(2 * numFrames);
        sndFile.readf(tempReadBuffer->data(), numFrames);
        output.readInterleaved(*tempReadBuffer);
    }
}

std::optional<sfz::FilePool::FileInformation> sfz::FilePool::getFileInformation(std::string_view filename)
{
    std::filesystem::path file { rootDirectory / filename };
    if (!std::filesystem::exists(file))
        return {};

    SndfileHandle sndFile(reinterpret_cast<const char*>(file.c_str()));
    if (sndFile.channels() != 1 && sndFile.channels() != 2) {
        DBG("Missing logic for " << sndFile.channels() <<", discarding sample " << filename);
        return {};
    }
    FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(sndFile.frames());
    returnedValue.sampleRate = static_cast<double>(sndFile.samplerate());
    SF_INSTRUMENT instrumentInfo;
    sndFile.command(SFC_GET_INSTRUMENT, &instrumentInfo, sizeof(instrumentInfo));
    if (instrumentInfo.loop_count == 1) {
        returnedValue.loopBegin = instrumentInfo.loops[0].start;
        returnedValue.loopEnd = instrumentInfo.loops[0].end;
    }

    const auto preloadedSize = [&]() {
        if (config::preloadSize == 0)
            return returnedValue.end;
        else
            return std::min(returnedValue.end, static_cast<uint32_t>(config::preloadSize));
    }();
    returnedValue.preloadedData = std::make_shared<StereoBuffer<float>>(preloadedSize);
    preloadedData[filename] = returnedValue.preloadedData;
    readFromFile(sndFile, preloadedSize, *returnedValue.preloadedData);
    // char  buffer [2048] ;
    // sndFile.command(SFC_GET_LOG_INFO, buffer, sizeof(buffer)) ;
    // DBG(buffer);
    return returnedValue;
}

void sfz::FilePool::enqueueLoading(Voice* voice, std::string_view sample, int numFrames)
{
    if (!loadingQueue.try_enqueue({ voice, sample, numFrames })) {
        DBG("Problem enqueuing a file read for file " << sample);
    }
}

void sfz::FilePool::loadingThread()
{
    FileLoadingInformation fileToLoad {};
    while (!quitThread) {
        if (!loadingQueue.wait_dequeue_timed(fileToLoad, 1ms))
            continue;

        if (fileToLoad.voice == nullptr) {
            DBG("Background thread error: voice is null.");
            continue;
        }

        DBG("Background loading of: " << fileToLoad.sample);
        std::filesystem::path file { rootDirectory / fileToLoad.sample };
        if (!std::filesystem::exists(file)) {
            DBG("Background thread: no file " << fileToLoad.sample << " exists.");
            continue;
        }

        SndfileHandle sndFile(reinterpret_cast<const char*>(file.c_str()));
        auto fileLoaded = std::make_unique<StereoBuffer<float>>(fileToLoad.numFrames);
        readFromFile(sndFile, fileToLoad.numFrames, *fileLoaded);
        fileToLoad.voice->setFileData(std::move(fileLoaded));
    }
}