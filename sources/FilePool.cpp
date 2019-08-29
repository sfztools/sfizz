#include "FilePool.h"
#include "Globals.h"
#include "absl/types/span.h"
#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
using namespace std::chrono_literals;

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
    // auto tempReadBuffer = std::make_unique<Buffer<float>>(2 * preloadedSize);
    if (sndFile.channels() == 1) {
        tempReadBuffer.resize(preloadedSize);
        sndFile.readf(tempReadBuffer.data(), preloadedSize);
        std::copy(tempReadBuffer.begin(), tempReadBuffer.end(), returnedValue.preloadedData->begin(Channel::left));
        std::copy(tempReadBuffer.begin(), tempReadBuffer.end(), returnedValue.preloadedData->begin(Channel::right));
    } else if (sndFile.channels() == 2) {
        tempReadBuffer.resize(preloadedSize * 2);
        sndFile.readf(tempReadBuffer.data(), preloadedSize);
        returnedValue.preloadedData->readInterleaved(tempReadBuffer);
    }
    // std::stringstream dumpName { };
    // dumpName << filename <<  ".preloaded" << preloadedFilesWritten << ".bin";
    // dumpName << filename <<  ".preloaded" << ".bin";
    // std::ofstream ofile(dumpName.str(), std::ios::binary);
    // preloadedFilesWritten++;
    // for (auto& floatValue: tempReadBuffer)
    //     ofile.write((char*) &floatValue, sizeof(float));
    // ofile.close();
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
        if (sndFile.channels() == 1) {
            tempReadBuffer.resize(fileToLoad.numFrames);
            sndFile.readf(tempReadBuffer.data(), fileToLoad.numFrames);
            std::copy(tempReadBuffer.begin(), tempReadBuffer.end(), fileLoaded->begin(Channel::left));
            std::copy(tempReadBuffer.begin(), tempReadBuffer.end(), fileLoaded->begin(Channel::right));
        } else if (sndFile.channels() == 2) {
            tempReadBuffer.resize(fileToLoad.numFrames * 2);
            sndFile.readf(tempReadBuffer.data(), fileToLoad.numFrames);
            fileLoaded->readInterleaved(tempReadBuffer);
        }
        // std::unique_ptr<StereoBuffer<float>, std::function<void(StereoBuffer<float>*)>> fileLoaded(new StereoBuffer<float>(framesRead), deleteAndTrackBuffers);
        // fileBuffers++;
        fileToLoad.voice->setFileData(std::move(fileLoaded));
    }
}