// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PuglHelpers.h"
#include <stdexcept>
//#include <mutex>

static std::weak_ptr<SingletonPuglWorld> gWorld;
//static std::mutex gWorldMutex;

std::shared_ptr<SingletonPuglWorld> SingletonPuglWorld::instance()
{
    //std::lock_guard<std::mutex> lock { gWorldMutex };

    std::shared_ptr<SingletonPuglWorld> world = gWorld.lock();
    if (world)
        return world;

    world.reset(new SingletonPuglWorld);
    world->world_.reset(puglNewWorld(PUGL_MODULE, PUGL_WORLD_THREADS));

    if (!world->world_)
        throw std::runtime_error("Failed to instantiate the graphical world.");

    gWorld = world;
    return world;
}
