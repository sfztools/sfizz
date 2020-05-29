// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Res.h"
#include "whereami.h"
#include <shared_mutex>
#include <memory>
#include <cstdio>

namespace Res {

static std::string gRootPath;

// mutex, maybe not necessary
#if __cplusplus >= 201703L
typedef std::shared_mutex RwLock;
#else
typedef std::shared_timed_mutex RwLock;
#endif
static RwLock gRootPathMutex;

std::string getPath(const char* relativePath)
{
    std::shared_lock<RwLock> readLock { gRootPathMutex };

    const std::string& rootPath = gRootPath;
    if (rootPath.empty())
        return {};

    return rootPath + relativePath;
}

void initializeRootPath(const char* rootPath)
{
    std::lock_guard<RwLock> writeLock { gRootPathMutex };

    gRootPath = std::string(rootPath) + '/';

    fprintf(stderr, "[sfizz] explicit resource path: %s\n", gRootPath.c_str());
}

static std::string currentModuleDir()
{
    unsigned length = wai_getModulePath(nullptr, 0, nullptr);
    if (static_cast<int>(length) == -1)
        return {};

    std::unique_ptr<char[]> mem { new char[length] };
    unsigned dirlen = 0;

    unsigned ret = wai_getModulePath(mem.get(), length, reinterpret_cast<int*>(&dirlen));
    if (ret != length)
        return {};

    return std::string(mem.get(), dirlen);
}

void initializeRootPathFromCurrentModule(const char* pathSuffix)
{
    std::lock_guard<RwLock> writeLock { gRootPathMutex };

    std::string moduleDir = currentModuleDir();
    if (moduleDir.empty())
        return;

    gRootPath = moduleDir + '/' + pathSuffix + '/';

    fprintf(stderr, "[sfizz] resource path from module: %s\n", gRootPath.c_str());
}

} // namespace Res
