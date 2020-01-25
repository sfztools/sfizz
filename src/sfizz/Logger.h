// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

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
