// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "LeakDetector.h"
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

/**
 * @brief Creates an RAII logger which fills or adds to a duration on destruction
 *
 */
struct ScopedTiming
{
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    enum class Operation
    {
        addToDuration,
        replaceDuration
    };
    ScopedTiming() = delete;
    /**
     * @brief Construct a new Scoped Logger object
     *
     * @param targetDuration
     * @param op
     */
    ScopedTiming(Duration& targetDuration, Operation op = Operation::replaceDuration);
    ~ScopedTiming();
    Duration& targetDuration;
    const Operation operation;
    const TimePoint creationTime { std::chrono::high_resolution_clock::now() };
};

struct FileTime
{
    FileTime() = default;
    FileTime(Duration waitDuration, Duration loadDuration, uint32_t fileSize, absl::string_view filename)
    : waitDuration(waitDuration), loadDuration(loadDuration), fileSize(fileSize), filename(filename) { }
    FileTime(const FileTime&) = default;
    FileTime& operator=(const FileTime&) = default;
    FileTime(FileTime&&) = default;
    FileTime& operator=(FileTime&&) = default;
    Duration waitDuration { 0 };
    Duration loadDuration { 0 };
    uint32_t fileSize { 0 };
    absl::string_view filename {};
    LEAK_DETECTOR(FileTime);
};

struct CallbackBreakdown
{
    Duration dispatch { 0 };
    Duration renderMethod { 0 };
    Duration data { 0 };
    Duration amplitude { 0 };
    Duration filters { 0 };
    Duration panning { 0 };
    Duration effects { 0 };
    LEAK_DETECTOR(CallbackBreakdown);
};

struct CallbackTime
{
    CallbackTime() = default;
    CallbackTime(const CallbackBreakdown& breakdown, int numVoices, size_t numSamples)
    : breakdown(breakdown), numVoices(numVoices), numSamples(numSamples) { }
    CallbackTime(const CallbackTime&) = default;
    CallbackTime& operator=(const CallbackTime&) = default;
    CallbackTime(CallbackTime&&) = default;
    CallbackTime& operator=(CallbackTime&&) = default;
    CallbackBreakdown breakdown {};
    int numVoices { 0 };
    size_t numSamples { 0 };
    LEAK_DETECTOR(CallbackTime);
};

class Logger
{
public:
    Logger();
    ~Logger();
    /**
     * @brief Set the prefix for the output log files
     *
     * @param prefix
     */
    void setPrefix(const std::string& prefix);

    /**
     * @brief Removes all logged data
     *
     */
    void clear();

    /**
     * @brief Enables logging and writing to log files on destruction
     *
     */
    void enableLogging();

    /**
     * @brief Disables logging and writing to log files on destruction
     *
     */
    void disableLogging();

    /**
     * @brief Logs the callback duration, with breakdown per operations
     *
     * @param breakdown The different timings for the callback
     * @param numVoices The number of active voices
     * @param numSamples The number of samples in the callback
     */
    void logCallbackTime(const CallbackBreakdown& breakdown, int numVoices, size_t numSamples);

    /**
     * @brief Log a file loading and waiting duration
     *
     * @param waitDuration The time spent waiting before loading the file
     * @param loadDuration The time it took to load the file
     * @param fileSize The file size
     * @param filename The file name
     */
    void logFileTime(Duration waitDuration, Duration loadDuration, uint32_t fileSize, absl::string_view filename);
private:
    /**
     * @brief Move all events from the real time queues to the non-realtime vectors
     */
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
    LEAK_DETECTOR(Logger);
};

}
