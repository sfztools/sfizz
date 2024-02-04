// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
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

#include "FilePool.h"
#include "AudioReader.h"
#include "Buffer.h"
#include "AudioBuffer.h"
#include "AudioSpan.h"
#include "Config.h"
#include "utility/SwapAndPop.h"
#include "utility/Debug.h"
#include <ThreadPool.h>
#include <absl/types/span.h>
#include <absl/strings/match.h>
#include <absl/memory/memory.h>
#include <algorithm>
#include <memory>
#include <thread>
#include <system_error>
#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif
using namespace std::placeholders;

struct sfz::FilePool::GlobalObject
{
    GlobalObject(size_t numThreads)
    : threadPool(numThreads)
    {}

    ThreadPool threadPool;

    // Preloaded data
    absl::flat_hash_map<FileId, std::shared_ptr<FileData>> preloadedFiles;
    absl::flat_hash_map<FileId, std::shared_ptr<FileData>> loadedFiles;

    SpinMutex preloadedFilesMutex;
    SpinMutex loadedFilesMutex;
};

static std::weak_ptr<sfz::FilePool::GlobalObject> globalObjectWeakPtr;
static std::mutex globalObjectMutex;

static std::shared_ptr<sfz::FilePool::GlobalObject> globalObject()
{
    std::shared_ptr<sfz::FilePool::GlobalObject> object;

    object = globalObjectWeakPtr.lock();
    if (object)
        return object;

    std::lock_guard<std::mutex> guard { globalObjectMutex };
    object = globalObjectWeakPtr.lock();
    if (object)
        return object;

    unsigned numThreads = std::thread::hardware_concurrency();
    numThreads = (numThreads > 2) ? (numThreads - 2) : 1;
    object.reset(new sfz::FilePool::GlobalObject(numThreads));
    globalObjectWeakPtr = object;
    return object;
}

void readBaseFile(sfz::AudioReader& reader, sfz::FileAudioBuffer& output, uint32_t numFrames)
{
    output.reset();
    output.resize(numFrames);

    const unsigned channels = reader.channels();

    if (channels == 1) {
        output.addChannel();
        output.clear();
        reader.readNextBlock(output.channelWriter(0), numFrames);
    } else if (channels == 2) {
        output.addChannel();
        output.addChannel();
        output.clear();
        sfz::Buffer<float> tempReadBuffer { 2 * numFrames };
        reader.readNextBlock(tempReadBuffer.data(), numFrames);
        sfz::readInterleaved(tempReadBuffer, output.getSpan(0), output.getSpan(1));
    }
}

sfz::FileAudioBuffer readFromFile(sfz::AudioReader& reader, uint32_t numFrames)
{
    sfz::FileAudioBuffer baseBuffer;
    readBaseFile(reader, baseBuffer, numFrames);
    return baseBuffer;
}

void streamFromFile(sfz::AudioReader& reader, sfz::FileAudioBuffer& output, std::atomic<size_t>* filledFrames = nullptr)
{
    const auto numFrames = static_cast<size_t>(reader.frames());
    const auto numChannels = reader.channels();
    const auto chunkSize = static_cast<size_t>(sfz::config::fileChunkSize);

    output.reset();
    output.addChannels(reader.channels());
    output.resize(numFrames);
    output.clear();

    sfz::Buffer<float> fileBlock { chunkSize * numChannels };
    size_t inputFrameCounter { 0 };
    size_t outputFrameCounter { 0 };
    bool inputEof = false;

    while (!inputEof && inputFrameCounter < numFrames)
    {
        auto thisChunkSize = std::min(chunkSize, numFrames - inputFrameCounter);
        const auto numFramesRead = static_cast<size_t>(
            reader.readNextBlock(fileBlock.data(), thisChunkSize));
        if (numFramesRead == 0)
            break;

        if (numFramesRead < thisChunkSize) {
            inputEof = true;
            thisChunkSize = numFramesRead;
        }
        const auto outputChunkSize = thisChunkSize;

        for (size_t chanIdx = 0; chanIdx < numChannels; chanIdx++) {
            const auto outputChunk = output.getSpan(chanIdx).subspan(outputFrameCounter, outputChunkSize);
            for (size_t i = 0; i < thisChunkSize; ++i)
                outputChunk[i] = fileBlock[i * numChannels + chanIdx];
        }
        inputFrameCounter += thisChunkSize;
        outputFrameCounter += outputChunkSize;

        if (filledFrames != nullptr)
            filledFrames->fetch_add(outputChunkSize);
    }
}

sfz::FilePool::FilePool()
    : filesToLoad(alignedNew<FileQueue>()),
      globalObj(globalObject())
{
    loadingJobs.reserve(config::maxVoices);
    lastUsedFiles.reserve(config::maxVoices);
}

sfz::FilePool::~FilePool()
{
    resetPreloadCallCounts();
    removeUnusedPreloadedData();

    std::error_code ec;

    garbageFlag = false;
    semGarbageBarrier.post(ec);
    garbageThread.join();

    dispatchFlag = false;
    dispatchBarrier.post(ec);
    dispatchThread.join();

    for (auto& job : loadingJobs)
        job.wait();
}

bool sfz::FilePool::checkSample(std::string& filename) const noexcept
{
    fs::path path { rootDirectory / filename };
    std::error_code ec;
    if (fs::exists(path, ec))
        return true;

#if defined(_WIN32)
    return false;
#else
    fs::path oldPath = std::move(path);
    path = oldPath.root_path();

    static const fs::path dot { "." };
    static const fs::path dotdot { ".." };

    for (const fs::path& part : oldPath.relative_path()) {
        if (part == dot || part == dotdot) {
            path /= part;
            continue;
        }

        if (fs::exists(path / part, ec)) {
            path /= part;
            continue;
        }

        auto it = path.empty() ? fs::directory_iterator { dot, ec } : fs::directory_iterator { path, ec };
        if (ec) {
            DBG("Error creating a directory iterator for " << filename << " (Error code: " << ec.message() << ")");
            return false;
        }

        auto searchPredicate = [&part](const fs::directory_entry &ent) -> bool {
#if !defined(GHC_USE_WCHAR_T)
            return absl::EqualsIgnoreCase(
                ent.path().filename().native(), part.native());
#else
            return absl::EqualsIgnoreCase(
                ent.path().filename().u8string(), part.u8string());
#endif
        };

        while (it != fs::directory_iterator {} && !searchPredicate(*it))
            it.increment(ec);

        if (it == fs::directory_iterator {}) {
            DBG("File not found, could not resolve " << filename);
            return false;
        }

        path /= it->path().filename();
    }

    const auto newPath = fs::relative(path, rootDirectory, ec);
    if (ec) {
        DBG("Error extracting the new relative path for " << filename << " (Error code: " << ec.message() << ")");
        return false;
    }
    DBG("Updating " << filename << " to " << newPath);
    filename = newPath.string();
    return true;
#endif
}

bool sfz::FilePool::checkSampleId(FileId& fileId) const noexcept
{
    {
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        if (globalObj->loadedFiles.contains(fileId))
            return true;
    }

    std::string filename = fileId.filename();
    bool result = checkSample(filename);
    if (result)
        fileId = FileId(std::move(filename), fileId.isReverse());
    return result;
}

absl::optional<sfz::FileInformation> getReaderInformation(sfz::AudioReader* reader) noexcept
{
    const unsigned channels = reader->channels();
    if (channels != 1 && channels != 2)
        return {};

    sfz::FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(reader->frames()) - 1;
    returnedValue.sampleRate = static_cast<double>(reader->sampleRate());
    returnedValue.numChannels = static_cast<int>(channels);

    // Check for instrument info
    sfz::InstrumentInfo instrumentInfo {};
    if (reader->getInstrumentInfo(instrumentInfo)) {
        returnedValue.rootKey = clamp<uint8_t>(instrumentInfo.basenote, 0, 127);
        if (reader->type() == sfz::AudioReaderType::Forward) {
            if (instrumentInfo.loop_count > 0) {
                returnedValue.hasLoop = true;
                returnedValue.loopStart = instrumentInfo.loops[0].start;
                returnedValue.loopEnd =
                    min(returnedValue.end, static_cast<int64_t>(instrumentInfo.loops[0].end - 1));
            }
        } else {
            // TODO loops ignored when reversed
            //   prehaps it can make use of SF_LOOP_BACKWARD?
        }
    }

    // Check for wavetable info
    sfz::WavetableInfo wt {};
    if (reader->getWavetableInfo(wt))
        returnedValue.wavetable = wt;

    return returnedValue;
}

absl::optional<sfz::FileInformation> sfz::FilePool::checkExistingFileInformation(const FileId& fileId) noexcept
{
    {
        auto& loadedFiles = globalObj->loadedFiles;
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        const auto loadedFile = loadedFiles.find(fileId);
        if (loadedFile != loadedFiles.end())
            return loadedFile->second->information;
    }

    {
        auto& preloadedFiles = globalObj->preloadedFiles;
        const auto preloadedFile = preloadedFiles.find(fileId);
        if (preloadedFile != preloadedFiles.end())
            return preloadedFile->second->information;
    }

    return {};
}

absl::optional<sfz::FileInformation> sfz::FilePool::getFileInformation(const FileId& fileId) noexcept
{
    auto existingInformation = checkExistingFileInformation(fileId);
    if (existingInformation)
        return existingInformation;

    const fs::path file { rootDirectory / fileId.filename() };

    if (!fs::exists(file))
        return {};

    AudioReaderPtr reader = createAudioReader(file, fileId.isReverse());
    return getReaderInformation(reader.get());
}

bool sfz::FilePool::preloadFile(const FileId& fileId, uint32_t maxOffset) noexcept
{
    {
        auto& loadedFiles = globalObj->loadedFiles;
        auto& mutex = globalObj->loadedFilesMutex;
        mutex.lock();
        const auto loadedFile = loadedFiles.find(fileId);
        if (loadedFile != loadedFiles.end()) {
            auto fileData = loadedFile->second;
            mutex.unlock();
            if (fileData->incrPreloadCallCount(this)) {
                numPreloadedSamples++;
            }
            return true;
        }
        mutex.unlock();
    }

    auto fileInformation = getFileInformation(fileId);
    if (!fileInformation)
        return false;

    fileInformation->maxOffset = maxOffset;
    const fs::path file { rootDirectory / fileId.filename() };
    AudioReaderPtr reader = createAudioReader(file, fileId.isReverse());

    const auto frames = static_cast<uint32_t>(reader->frames());
    const auto framesToLoad = [&]() {
        if (loadInRam)
            return frames;
        else
            return min(frames, maxOffset + preloadSize);
    }();

    auto& mutex = globalObj->preloadedFilesMutex;
    auto& preloadedFiles = globalObj->preloadedFiles;
    mutex.lock();
    const auto existingFile = preloadedFiles.find(fileId);
    if (existingFile != preloadedFiles.end()) {
        std::shared_ptr<FileData> fileData = existingFile->second;
        mutex.unlock();
        std::lock_guard<std::mutex> guard { fileData->readMutex };
        if (framesToLoad > fileData->preloadedData.getNumFrames()) {
            fileData->information.maxOffset = maxOffset;
            fileData->preloadedData = readFromFile(*reader, framesToLoad);
        }
        if (fileData->incrPreloadCallCount(this)) {
            numPreloadedSamples++;
        }
    } else {
        auto insertedPair = preloadedFiles.insert_or_assign(fileId, std::shared_ptr<FileData>(new FileData()));
        std::shared_ptr<FileData> fileData = insertedPair.first->second;
        mutex.unlock();

        fileInformation->sampleRate = static_cast<double>(reader->sampleRate());
        fileData->copyFrom(FileData(
            readFromFile(*reader, framesToLoad),
            *fileInformation
        ));

        fileData->status = FileData::Status::Preloaded;
        if (fileData->incrPreloadCallCount(this)) {
            numPreloadedSamples++;
        }
        fileData->readMutex.unlock();
    }

    return true;
}

void sfz::FilePool::resetPreloadCallCounts() noexcept
{
    {
        std::lock_guard<SpinMutex> guard { globalObj->preloadedFilesMutex };
        auto& preloadedFiles = globalObj->preloadedFiles;
        for (auto& preloadedFile: preloadedFiles)
            preloadedFile.second->resetPreloadCallCount(this);
    }

    {
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        auto& loadedFiles = globalObj->loadedFiles;
        for (auto& loadedFile: loadedFiles)
            loadedFile.second->resetPreloadCallCount(this);
    }
}

void sfz::FilePool::removeUnusedPreloadedData() noexcept
{
    {
        std::lock_guard<SpinMutex> guard { globalObj->preloadedFilesMutex };
        auto& preloadedFiles = globalObj->preloadedFiles;
        for (auto it = preloadedFiles.begin(), end = preloadedFiles.end(); it != end; it++) {
            auto& fileData = it->second;
            if (fileData->forgetReference(this)) {
                numPreloadedSamples--;
                DBG("[sfizz] Removing unused preloaded data: " << it->first.filename());
                if (fileData->getPreloadCallCount() == 0)
                    preloadedFiles.erase(it);
            }
        }
    }

    {
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        auto& loadedFiles = globalObj->loadedFiles;
        for (auto it = loadedFiles.begin(), end = loadedFiles.end(); it != end; it++) {
            auto& fileData = it->second;
            if (fileData->forgetReference(this)) {
                numPreloadedSamples--;
                DBG("[sfizz] Removing unused loaded data: " << it->first.filename());
                if (fileData->getPreloadCallCount() == 0)
                    loadedFiles.erase(it);
            }
        }
    }
}

sfz::FileDataHolder sfz::FilePool::loadFile(const FileId& fileId) noexcept
{
    auto fileInformation = getFileInformation(fileId);
    if (!fileInformation)
        return {};

    auto& mutex = globalObj->loadedFilesMutex;
    auto& loadedFiles = globalObj->loadedFiles;
    mutex.lock();
    const auto existingFile = loadedFiles.find(fileId);
    if (existingFile != loadedFiles.end()) {
        std::shared_ptr<FileData> fileData = existingFile->second;
        mutex.unlock();
        if (fileData->incrPreloadCallCount(this)) {
            numPreloadedSamples++;
        }
        return { fileData };
    }
    auto insertedPair = loadedFiles.insert_or_assign(fileId, std::shared_ptr<FileData>(new FileData()));
    std::shared_ptr<FileData> fileData = insertedPair.first->second;
    mutex.unlock();

    const fs::path file { rootDirectory / fileId.filename() };
    AudioReaderPtr reader = createAudioReader(file, fileId.isReverse());

    const auto frames = static_cast<uint32_t>(reader->frames());
    fileData->copyFrom(FileData(
        readFromFile(*reader, frames),
        *fileInformation
    ));
    fileData->status = FileData::Status::Preloaded;
    if (fileData->incrPreloadCallCount(this)) {
        numPreloadedSamples++;
    }
    fileData->readMutex.unlock();
    return { fileData };
}

sfz::FileDataHolder sfz::FilePool::loadFromRam(const FileId& fileId, const std::vector<char>& data) noexcept
{
    auto& mutex = globalObj->loadedFilesMutex;
    auto& loadedFiles = globalObj->loadedFiles;
    mutex.lock();
    const auto loaded = loadedFiles.find(fileId);
    if (loaded != loadedFiles.end()) {
        std::shared_ptr<FileData> fileData = loaded->second;
        mutex.unlock();
        if (fileData->incrPreloadCallCount(this)) {
            numPreloadedSamples++;
        }
        return { fileData };
    }

    auto insertedPair = loadedFiles.insert_or_assign(fileId, std::shared_ptr<FileData>(new FileData()));
    std::shared_ptr<FileData> fileData = insertedPair.first->second;
    mutex.unlock();
    auto reader = createAudioReaderFromMemory(data.data(), data.size(), fileId.isReverse());
    auto fileInformation = getReaderInformation(reader.get());
    const auto frames = static_cast<uint32_t>(reader->frames());
    fileData->copyFrom(FileData(
        readFromFile(*reader, frames),
        *fileInformation
    ));
    fileData->status = FileData::Status::Preloaded;
    if (fileData->incrPreloadCallCount(this)) {
        numPreloadedSamples++;
    }
    DBG("Added a file " << fileId.filename());
    fileData->readMutex.unlock();
    return { fileData };
}

sfz::FileDataHolder sfz::FilePool::getFilePromise(const std::shared_ptr<FileId>& fileId) noexcept
{
    {
        auto& loadedFiles = globalObj->loadedFiles;
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        const auto loaded = loadedFiles.find(*fileId);
        if (loaded != loadedFiles.end())
            return { loaded->second };
    }

    auto& preloadedFiles = globalObj->preloadedFiles;
    auto& mutex = globalObj->preloadedFilesMutex;
    mutex.lock();
    const auto preloaded = preloadedFiles.find(*fileId);
    if (preloaded == preloadedFiles.end()) {
        mutex.unlock();
        DBG("[sfizz] File not found in the preloaded files: " << fileId->filename());
        return {};
    }
    std::shared_ptr<FileData> fileData = preloaded->second;
    mutex.unlock();

    if (!loadInRam) {
        QueuedFileData queuedData { fileId, fileData };
        if (!filesToLoad->try_push(queuedData)) {
            DBG("[sfizz] Could not enqueue the file to load for " << fileId << " (queue capacity " << filesToLoad->capacity() << ")");
            return {};
        }

        std::error_code ec;
        dispatchBarrier.post(ec);
        ASSERT(!ec);
    }

    return { fileData };
}

void sfz::FilePool::setPreloadSize(uint32_t preloadSize) noexcept
{
    this->preloadSize = preloadSize;
    if (loadInRam)
        return;

    // Update all the preloaded sizes
    auto& mutex = globalObj->preloadedFilesMutex;
    mutex.lock();
    auto preloadedFiles = globalObj->preloadedFiles; // copying map. don't use "auto&".
    mutex.unlock();
    for (auto& preloadedFile : preloadedFiles) {
        auto& fileId = preloadedFile.first;
        auto& fileData = preloadedFile.second;
        std::lock_guard<std::mutex> guard { fileData->readMutex };
        const auto maxOffset = fileData->information.maxOffset;
        fs::path file { rootDirectory / fileId.filename() };
        AudioReaderPtr reader = createAudioReader(file, fileId.isReverse());
        fileData->preloadedData = readFromFile(*reader, preloadSize + maxOffset);
    }
}

void sfz::FilePool::loadingJob(const QueuedFileData& data) noexcept
{
    raiseCurrentThreadPriority();

    std::shared_ptr<FileId> id = data.id.lock();
    if (!id) {
        // file ID was nulled, it means the region was deleted, ignore
        return;
    }
    {
        const fs::path file { rootDirectory / id->filename() };
        std::error_code readError;
        AudioReaderPtr reader = createAudioReader(file, id->isReverse(), &readError);

        if (readError) {
            DBG("[sfizz] reading the file errored for " << *id << " with code " << readError << ": " << readError.message());
            return;
        }

        std::lock_guard<std::mutex> guard { data.data->readMutex };
        FileData::Status currentStatus = data.data->status.load();

        unsigned spinCounter { 0 };
        while (currentStatus == FileData::Status::Invalid) {
            // Spin until the state changes
            if (spinCounter > 1024) {
                DBG("[sfizz] " << *id << " is stuck on Invalid? Leaving the load");
                return;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
            currentStatus = data.data->status.load();
            spinCounter += 1;
        }

        // Already loading or loaded
        if (currentStatus != FileData::Status::Preloaded)
            return;

        // Someone else got the token
        if (!data.data->status.compare_exchange_strong(currentStatus, FileData::Status::Streaming))
            return;

        streamFromFile(*reader, data.data->fileData, &data.data->availableFrames);

        data.data->status = FileData::Status::Done;
    }

    std::lock_guard<SpinMutex> guard { lastUsedMutex };
    if (absl::c_find(lastUsedFiles, *id) == lastUsedFiles.end())
        lastUsedFiles.push_back(*id);
}

void sfz::FilePool::clear()
{
    std::lock_guard<SpinMutex> guard { lastUsedMutex };
    emptyFileLoadingQueues();
    lastUsedFiles.clear();
    resetPreloadCallCounts();
    removeUnusedPreloadedData();
}

uint32_t sfz::FilePool::getPreloadSize() const noexcept
{
    return preloadSize;
}

size_t sfz::FilePool::getActualNumPreloadedSamples() const
{
    size_t s = 0;
    {
        std::lock_guard<SpinMutex> guard { globalObj->preloadedFilesMutex };
        for (const auto& pair : globalObj->preloadedFiles) {
            if (pair.second->contains(this)) {
                ++s;
            }
        }
    }
    {
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        for (const auto& pair : globalObj->loadedFiles) {
            if (pair.second->contains(this)) {
                ++s;
            }
        }
    }
    return s;
}

size_t sfz::FilePool::getGlobalNumPreloadedSamples() const
{
    size_t s = 0;
    {
        std::lock_guard<SpinMutex> guard { globalObj->preloadedFilesMutex };
        s += globalObj->preloadedFiles.size();
    }
    {
        std::lock_guard<SpinMutex> guard { globalObj->loadedFilesMutex };
        s += globalObj->loadedFiles.size();
    }
    return s;
}

template <typename R>
bool is_ready(std::future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void sfz::FilePool::dispatchingJob() noexcept
{
    while (dispatchBarrier.wait(), dispatchFlag) {
        std::lock_guard<std::mutex> guard { loadingJobsMutex };

        QueuedFileData queuedData;
        if (filesToLoad->try_pop(queuedData)) {
            if (queuedData.id.expired()) {
                // file ID was nulled, it means the region was deleted, ignore
            }
            else
                loadingJobs.push_back(
                    globalObj->threadPool.enqueue([this](const QueuedFileData& data) { loadingJob(data); }, std::move(queuedData)));
        }

        // Clear finished jobs
        swapAndPopAll(loadingJobs, [](std::future<void>& future) {
            return is_ready(future);
        });
    }
}

void sfz::FilePool::waitForBackgroundLoading() noexcept
{
    std::lock_guard<std::mutex> guard { loadingJobsMutex };

    for (auto& job : loadingJobs)
        job.wait();

    loadingJobs.clear();
}

void sfz::FilePool::raiseCurrentThreadPriority() noexcept
{
#if defined(_WIN32)
    HANDLE thread = GetCurrentThread();
    const int priority = THREAD_PRIORITY_ABOVE_NORMAL; /*THREAD_PRIORITY_HIGHEST*/
    if (!SetThreadPriority(thread, priority)) {
        std::system_error error(GetLastError(), std::system_category());
        DBG("[sfizz] Cannot set current thread priority: " << error.what());
    }
#else
    pthread_t thread = pthread_self();
    int policy;
    sched_param param;

    if (pthread_getschedparam(thread, &policy, &param) != 0) {
        DBG("[sfizz] Cannot get current thread scheduling parameters");
        return;
    }

    policy = SCHED_RR;
    const int minprio = sched_get_priority_min(policy);
    const int maxprio = sched_get_priority_max(policy);
    param.sched_priority = minprio + config::backgroundLoaderPthreadPriority * (maxprio - minprio) / 100;

    if (pthread_setschedparam(thread, policy, &param) != 0) {
        DBG("[sfizz] Cannot set current thread scheduling parameters");
        return;
    }
#endif
}

void sfz::FilePool::setRamLoading(bool loadInRam) noexcept
{
    if (loadInRam == this->loadInRam)
        return;

    this->loadInRam = loadInRam;

    if (loadInRam) {
        auto& mutex = globalObj->preloadedFilesMutex;
        mutex.lock();
        auto preloadedFiles = globalObj->preloadedFiles; // copying map. don't use "auto&".
        mutex.unlock();
        for (auto& preloadedFile : preloadedFiles) {
            auto& fileId = preloadedFile.first;
            fs::path file { rootDirectory / fileId.filename() };
            AudioReaderPtr reader = createAudioReader(file, fileId.isReverse());
            auto& fileData = preloadedFile.second;
            std::lock_guard<std::mutex> guard { fileData->readMutex };
            fileData->preloadedData = readFromFile(
                *reader,
                fileData->information.end
            );
        }
    } else {
        setPreloadSize(preloadSize);
    }
}

void sfz::FilePool::triggerGarbageCollection() noexcept
{
    std::error_code ec;
    semGarbageBarrier.post(ec);
    ASSERT(!ec);
}

void sfz::FilePool::garbageJob() noexcept
{
    std::vector<FileAudioBuffer> garbageToCollect;
    garbageToCollect.reserve(config::maxVoices);
    while (semGarbageBarrier.wait(), garbageFlag) {
        auto& mutex1 = lastUsedMutex;
        if (!mutex1.try_lock()) {
            return;
        }
        auto lastUsedFilesTemp = lastUsedFiles;
        mutex1.unlock();

        auto& mutex2 = globalObj->preloadedFilesMutex;
        if (!mutex2.try_lock()) {
            return;
        }
        auto preloadedFiles = globalObj->preloadedFiles; // copying map. don't use "auto&".
        mutex2.unlock();

        const auto now = std::chrono::high_resolution_clock::now();
        swapAndPopAll(lastUsedFilesTemp, [&](const FileId& id) {
            if (garbageToCollect.size() == garbageToCollect.capacity())
                return false;

            auto it = preloadedFiles.find(id);
            if (it == preloadedFiles.end()) {
                // Getting here means that the preloadedFiles got changed (probably cleared)
                // while the lastUsedFiles were untouched.
                return true;
            }

            std::shared_ptr<FileData> data = it->second;

            const std::unique_lock<std::mutex> guard { data->readMutex, std::try_to_lock };
            if (!guard.owns_lock())
                return false;

            if (data->status == FileData::Status::Preloaded)
                return true;

            if (data->status != FileData::Status::Done)
                return false;

            if (data->readerCount != 0)
                return false;

            const auto secondsIdle = std::chrono::duration_cast<std::chrono::seconds>(now - data->lastViewerLeftAt).count();
            if (secondsIdle < config::fileClearingPeriod)
                return false;

            data->availableFrames = 0;
            data->status = FileData::Status::Preloaded;
            garbageToCollect.push_back(std::move(data->fileData));
            return true;
        });

        if (!garbageToCollect.empty()) {
            garbageToCollect.clear();
        }
    }
}
