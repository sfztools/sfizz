#include "Logger.h"
#include "absl/algorithm/container.h"
#include "ghc/fs_std.hpp"
#include <iostream>
#include <fstream>
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

sfz::Logger::~Logger()
{
    keepRunning.clear();
    loggingThread.join();

    if (!fileTimes.empty()) {
        fs::path loadLogPath{ fs::current_path() / "file_times.csv" };
        std::ofstream loadLogFile { loadLogPath.string() };
        loadLogFile << "WaitDuration,LoadDuration,FileSize,FileName" << '\n';
        for (auto& time: fileTimes)
            loadLogFile << time.waitDuration.count() << ','
                        << time.loadDuration.count() << ','
                        << time.fileSize << ','
                        << time.filename << '\n';
    }

    if (!callbackTimes.empty()) {
        fs::path callbackLogPath{ fs::current_path() / "callback_times.csv" };
        std::ofstream callbackLogFile { callbackLogPath.string() };
        callbackLogFile << "Duration,NumVoices,NumSamples" << '\n';
        for (auto& time: callbackTimes)
            callbackLogFile << time.duration.count() << ','
                            << time.numVoices << ','
                            << time.numSamples << '\n';
    }
}


void sfz::Logger::logCallbackTime(std::chrono::duration<double> duration, int numVoices, int numSamples)
{
    callbackTimeQueue.try_push<CallbackTime>({ duration, numVoices, numSamples });
}

void sfz::Logger::logFileTime(std::chrono::duration<double> waitDuration, std::chrono::duration<double> loadDuration, uint32_t fileSize, absl::string_view filename)
{
    fileTimeQueue.try_push<FileTime>({ waitDuration, loadDuration, fileSize, filename });
}

void sfz::Logger::moveEvents()
{
    while(keepRunning.test_and_set()) {
        CallbackTime callbackTime;
        while (callbackTimeQueue.try_pop(callbackTime))
            callbackTimes.push_back(callbackTime);

        sfz::FileTime fileTime;
        while (fileTimeQueue.try_pop(fileTime))
            fileTimes.push_back(fileTime);

        std::this_thread::sleep_for(50ms);
    }
}
