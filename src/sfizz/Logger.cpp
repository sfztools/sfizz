// SPDX-License-Identifier: BSD-2-Clause

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

#include "Logger.h"
#include "Debug.h"
#include "absl/algorithm/container.h"
#include "ghc/fs_std.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std::chrono_literals;

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
        DBG("Logging file times to " << fileLogPath.filename());
        std::ofstream loadLogFile { fileLogPath.string() };
        loadLogFile << "WaitDuration,LoadDuration,FileSize,FileName" << '\n';
        for (auto& time: fileTimes)
            loadLogFile << time.waitDuration.count() << ','
                        << time.loadDuration.count() << ','
                        << time.fileSize << ','
                        << time.filename << '\n';
    }

    if (!callbackTimes.empty()) {
        std::stringstream callbackLogFilename;
        callbackLogFilename << this << "_"
                            << prefix
                            << "_callback_log.csv";
        fs::path callbackLogPath{ fs::current_path() / callbackLogFilename.str() };
        DBG("Logging callback times to " << callbackLogPath.filename());
        std::ofstream callbackLogFile { callbackLogPath.string() };
        callbackLogFile << "Duration,NumVoices,NumSamples" << '\n';
        for (auto& time: callbackTimes)
            callbackLogFile << time.duration.count() << ','
                            << time.numVoices << ','
                            << time.numSamples << '\n';
    }
}


void sfz::Logger::logCallbackTime(std::chrono::duration<double> duration, int numVoices, size_t numSamples)
{
    if (!loggingEnabled)
        return;

    callbackTimeQueue.try_push<CallbackTime>({ duration, numVoices, numSamples });
}

void sfz::Logger::logFileTime(std::chrono::duration<double> waitDuration, std::chrono::duration<double> loadDuration, uint32_t fileSize, absl::string_view filename)
{
    if (!loggingEnabled)
        return;

    fileTimeQueue.try_push<FileTime>({ waitDuration, loadDuration, fileSize, filename });
}

void sfz::Logger::setPrefix(const std::string& prefix)
{
    this->prefix = prefix;
}

void sfz::Logger::clear()
{
    clearFlag.clear();
    prefix.clear();
}

void sfz::Logger::moveEvents() noexcept
{
    while(keepRunning.test_and_set()) {
        CallbackTime callbackTime;
        while (callbackTimeQueue.try_pop(callbackTime))
            callbackTimes.push_back(callbackTime);

        sfz::FileTime fileTime;
        while (fileTimeQueue.try_pop(fileTime))
            fileTimes.push_back(fileTime);

        if (!clearFlag.test_and_set()) {
            fileTimes.clear();
            callbackTimes.clear();
        }

        std::this_thread::sleep_for(50ms);
    }
}

void sfz::Logger::enableLogging()
{
    loggingEnabled = false;
}

void sfz::Logger::disableLogging()
{
    loggingEnabled = false;
    clearFlag.clear();
}
