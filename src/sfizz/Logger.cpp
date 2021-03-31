// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Logger.h"
#include "utility/Debug.h"
#include <ghc/fs_std.hpp>
#include <absl/algorithm/container.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

template<class T>
void printStatistics(std::vector<T>& data)
{
    absl::c_sort(data);
    const auto maxToTake = std::min(data.size(), size_t(10));
    std::vector<T> maxValues;
    auto it = data.rbegin();
    for (size_t i = 0; i < maxToTake; ++i)
        maxValues.push_back(*it++);

    const auto sum = absl::c_accumulate(data, 0.0);
    const auto size = static_cast<T>(data.size());
    const auto mean = sum / size;
    std::cout << "Mean: " << mean << '\n';
    if (data.size() > 1) {
        std::vector<T> squares;
        absl::c_transform(data, std::back_inserter(squares), [mean](T x) { return x * x; });
        const auto sumOfSquares = absl::c_accumulate(squares, 0.0);
        const auto variance = sumOfSquares / (size - 1.0) - (mean * mean) / size / (size - 1.0);
        std::cout << "Variance: " << variance << '\n';
        std::cout << "(Biased) deviation: " << std::sqrt(variance) << '\n';
    }

    std::cout << "Maximum values:";
    for (auto& value: maxValues)
        std::cout << value << ' ';
    std::cout << '\n';
}

sfz::Logger::Logger()
    : callbackTimeQueue(alignedNew<CallbackTimeQueue>()),
      fileTimeQueue(alignedNew<FileTimeQueue>())
{
    keepRunning.test_and_set();
    clearFlag.test_and_set();
    loggingThread = std::thread(&Logger::moveEvents, this);
}

sfz::Logger::~Logger()
{
    keepRunning.clear();
    loggingThread.join();

    if (!loggingEnabled)
        return;

    if (!fileTimes.empty()) {
        std::stringstream fileLogFilename;
        fileLogFilename << this << "_"
                        << prefix
                        << "_file_log.csv";
        fs::path fileLogPath{ fs::current_path() / fileLogFilename.str() };
        std::cout << "Logging " << fileTimes.size() << " file times to " << fileLogPath.filename() << '\n';
        std::ofstream loadLogFile { fileLogPath.string() };
        loadLogFile << "WaitDuration,LoadDuration,FileSize,FileName" << '\n';
        for (auto& time: fileTimes)
            loadLogFile << time.waitDuration.count() << ','
                        << time.loadDuration.count() << ','
                        << time.fileSize << ','
                        << "disabled" << '\n';
                        // << time.filename << '\n';
    }

    if (!callbackTimes.empty()) {
        std::stringstream callbackLogFilename;
        callbackLogFilename << this << "_"
                            << prefix
                            << "_callback_log.csv";
        fs::path callbackLogPath{ fs::current_path() / callbackLogFilename.str() };
        std::cout << "Logging " << callbackTimes.size() << " callback times to " << callbackLogPath.filename() << '\n';
        std::ofstream callbackLogFile { callbackLogPath.string() };
        callbackLogFile << "Dispatch,RenderMethod,Data,Amplitude,Filters,Panning,Effects,NumVoices,NumSamples" << '\n';
        for (auto& time: callbackTimes)
            callbackLogFile << time.breakdown.dispatch.count() << ','
                            << time.breakdown.renderMethod.count() << ','
                            << time.breakdown.data.count() << ','
                            << time.breakdown.amplitude.count() << ','
                            << time.breakdown.filters.count() << ','
                            << time.breakdown.panning.count() << ','
                            << time.breakdown.effects.count() << ','
                            << time.numVoices << ','
                            << time.numSamples << '\n';
    }
}


void sfz::Logger::logCallbackTime(const CallbackBreakdown& breakdown, int numVoices, size_t numSamples)
{
    if (!loggingEnabled)
        return;

    CallbackTime callbackTime;
    callbackTime.breakdown = breakdown;
    callbackTime.numVoices = numVoices;
    callbackTime.numSamples = numSamples;
    callbackTimeQueue->try_push(callbackTime);
}

void sfz::Logger::logFileTime(std::chrono::duration<double> waitDuration, std::chrono::duration<double> loadDuration, uint32_t fileSize, absl::string_view filename)
{
    if (!loggingEnabled)
        return;

    FileTime fileTime;
    fileTime.waitDuration = waitDuration;
    fileTime.loadDuration = loadDuration;
    fileTime.fileSize = fileSize;
    fileTime.filename = filename;
    fileTimeQueue->try_push(fileTime);
}

void sfz::Logger::setPrefix(absl::string_view prefix)
{
    this->prefix = std::string(prefix);
}

void sfz::Logger::clear()
{
    clearFlag.clear();
}

void sfz::Logger::moveEvents() noexcept
{
    while(keepRunning.test_and_set()) {
        CallbackTime callbackTime;
        while (callbackTimeQueue->try_pop(callbackTime))
            callbackTimes.push_back(callbackTime);

        FileTime fileTime;
        while (fileTimeQueue->try_pop(fileTime))
            fileTimes.push_back(fileTime);

        if (!clearFlag.test_and_set()) {
            fileTimes.clear();
            callbackTimes.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void sfz::Logger::enableLogging(absl::string_view prefix)
{
    setPrefix(prefix);
    loggingEnabled = true;
}

void sfz::Logger::disableLogging()
{
    loggingEnabled = false;
    clearFlag.clear();
}

sfz::ScopedTiming::ScopedTiming(Duration& targetDuration, Operation operation)
: targetDuration(targetDuration), operation(operation)
{

}

sfz::ScopedTiming::~ScopedTiming()
{
    switch(operation)
    {
    case(Operation::replaceDuration):
        targetDuration = std::chrono::high_resolution_clock::now() - creationTime;
        break;
    case(Operation::addToDuration):
        targetDuration += std::chrono::high_resolution_clock::now() - creationTime;
        break;
    }
}
