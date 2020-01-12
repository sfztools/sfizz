#include "Logger.h"
#include "absl/algorithm/container.h"
#include "ghc/fs_std.hpp"
#include <iostream>
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
    std::vector<T> squares;
    absl::c_transform(data, std::back_inserter(squares), [mean](T x) { return x * x; });
    const auto sumOfSquares = absl::c_accumulate(squares, 0.0);
    const auto variance = sumOfSquares / (size - 1.0) - (mean * mean) / size / (size - 1.0);
    std::cout << "Mean: " << mean << '\n';
    std::cout << "Variance: " << variance << '\n';
    std::cout << "(Biased) deviation: " << std::sqrt(variance) << '\n';
    std::cout << "Maximum values:";
    for (auto& value: maxValues)
        std::cout << value << ' ';
    std::cout << '\n';
}

sfz::Logger::~Logger()
{
    keepRunning.clear();
    loggingThread.join();

    if (!loadTimes.empty()) {
        std::vector<double> loadTimesStats;
        std::vector<double> normLoadTimesStats;
        absl::c_transform(loadTimes, std::back_inserter(loadTimesStats), [](auto x) { return x.value.count(); });
        absl::c_transform(loadTimes, std::back_inserter(normLoadTimesStats), [](auto x) {
            if (x.fileSize == 0)
                return x.value.count();

            return x.value.count() / static_cast<double>(x.fileSize);
        });
        std::cout << "\nFile load times" << '\n';
        printStatistics(loadTimesStats);
        std::cout << "\nNormalized file load times (per sample)" << '\n';
        printStatistics(normLoadTimesStats);
    }

    if (!waitTimes.empty()) {
        std::vector<double> waitTimesStats;
        absl::c_transform(waitTimes, std::back_inserter(waitTimesStats), [](auto x) { return x.count(); });
        std::cout << "\nWaiting times" << '\n';
        printStatistics(waitTimesStats);
    }

    if (!callbackTimes.empty()) {
        std::vector<double> callbackTimesStats;
        absl::c_transform(callbackTimes, std::back_inserter(callbackTimesStats), [](auto x) { return x.count(); });
        std::cout << "\nCallback times" << '\n';
        printStatistics(callbackTimesStats);
    }
}


void sfz::Logger::logCallbackTime(std::chrono::duration<double> value)
{
    callbackTimeQueue.try_push(value);
}
void sfz::Logger::logFileWaitTime(std::chrono::duration<double> value)
{
    fileWaitTimeQueue.try_push(value);
}
void sfz::Logger::logFileLoadTime(std::chrono::duration<double> value, uint32_t fileSize, absl::string_view filename)
{
    FileLoadTime toPush { value, fileSize, filename };
    fileLoadTimeQueue.try_push(toPush);
}

void sfz::Logger::moveEvents()
{
    while(keepRunning.test_and_set()) {
        std::chrono::duration<double> callbackTime;
        while (callbackTimeQueue.try_pop(callbackTime))
            callbackTimes.push_back(callbackTime);

        std::chrono::duration<double> waitTime;
        while (fileWaitTimeQueue.try_pop(waitTime))
            waitTimes.push_back(waitTime);

        sfz::FileLoadTime loadTime;
        while (fileLoadTimeQueue.try_pop(loadTime))
            loadTimes.push_back(loadTime);

        std::this_thread::sleep_for(50ms);
    }
}
