#include "Logger.h"
using namespace std::chrono_literals;
sfz::Logger::~Logger()
{
    keepRunning.clear();
    loggingThread.join();
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
