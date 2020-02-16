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
#include <functional>
#include <thread>
#include "absl/strings/string_view.h"

namespace sfz
{

using Duration = std::chrono::duration<double>;

struct ScopedLogger
{
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    ScopedLogger() = delete;
    ScopedLogger(std::function<void(Duration)> callback);
    ~ScopedLogger();
    const std::function<void(Duration)> callback;
    const TimePoint creationTime { std::chrono::high_resolution_clock::now() };
};

struct FileTime
{
    Duration waitDuration;
    Duration loadDuration;
    uint32_t fileSize;
    absl::string_view filename;
};

struct CallbackBreakdown
{
    Duration dispatch { 0 };
    Duration renderMethod { 0 };
    Duration data { 0 };
    Duration amplitude { 0 };
    Duration filters { 0 };
    Duration panning { 0 };
};

struct CallbackTime
{
    CallbackBreakdown breakdown;
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
    void logCallbackTime(CallbackBreakdown&& breakdown, int numVoices, size_t numSamples);
    void logFileTime(Duration waitDuration, Duration loadDuration, uint32_t fileSize, absl::string_view filename);
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
