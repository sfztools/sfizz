#pragma once
#include "StereoBuffer.h"
#include <atomic>
#include <memory>

namespace sfz
{
class Voice
{
public:
    void setFileData(std::unique_ptr<StereoBuffer<float>> file)
    {
        fileData = std::move(file);
        dataReady.store(true);
    }
private:
    std::atomic<bool> dataReady;
    std::unique_ptr<StereoBuffer<float>> fileData;
};
}