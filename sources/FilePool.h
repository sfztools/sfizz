#pragma once
#include "StereoBuffer.h"
#include <filesystem>
#include <map>
#include <string_view>

namespace sfz
{
class FilePool
{
public:
    FilePool() = default;
private:
    std::filesystem::path rootDirectory;
    std::map<std::string_view, std::shared_ptr<StereoBuffer<float>>> preloadedData;
};
}