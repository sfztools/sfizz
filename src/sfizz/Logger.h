// Copyright (c) 2019-2020, Paul Ferrand
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
#include "atomic_queue/atomic_queue.h"
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include "absl/strings/string_view.h"

namespace sfz
{

struct FileTime
{
    std::chrono::duration<double> waitDuration;
    std::chrono::duration<double> loadDuration;
    uint32_t fileSize;
    absl::string_view filename;
};

struct CallbackTime
{
    std::chrono::duration<double> duration;
    int numVoices;
    size_t numSamples;
};

class Logger
{
public:
    Logger();
    ~Logger();
    void setPrefix(const std::string& prefix);
    void clear();
    void enableLogging();
    void disableLogging();
    void logCallbackTime(std::chrono::duration<double> duration, int numVoices, size_t numSamples);
    void logFileTime(std::chrono::duration<double> waitDuration, std::chrono::duration<double> loadDuration, uint32_t fileSize, absl::string_view filename);
private:
    void moveEvents() noexcept;
    bool loggingEnabled { config::loggingEnabled };
    std::string prefix { "" };

    atomic_queue::AtomicQueue2<CallbackTime, config::loggerQueueSize, true, true, false, true> callbackTimeQueue;
    atomic_queue::AtomicQueue2<FileTime, config::loggerQueueSize, true, true, false, true> fileTimeQueue;
    std::vector<CallbackTime> callbackTimes;
    std::vector<FileTime> fileTimes;

    std::atomic_flag keepRunning;
    std::atomic_flag clearFlag;
    std::thread loggingThread;
};

}
