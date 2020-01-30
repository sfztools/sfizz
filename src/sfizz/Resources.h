// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

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
