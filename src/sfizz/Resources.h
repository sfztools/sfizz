#pragma once
#include "FilePool.h"
#include "Logger.h"

namespace sfz
{
struct Resources
{
    FilePool filePool;
    Logger logger;
};
}
