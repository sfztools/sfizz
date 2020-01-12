#pragma once
#include "FilePool.h"
#include "Logger.h"

namespace sfz
{
struct Resources
{
    Logger logger;
    FilePool filePool { logger };
};
}
