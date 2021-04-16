// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "catch2/catch.hpp"
#include <SpinMutex.h>
#include <thread>
#include <mutex>
#include <condition_variable>

TEST_CASE("[SpinMutex] Basic synchronization")
{
    constexpr size_t num_threads = 8;
    constexpr size_t num_iterations = 1000000;

    volatile size_t counter = 0;
    SpinMutex counter_mutex;

    std::thread threads[num_threads];

    volatile bool ready = false;
    std::condition_variable ready_cv;
    std::mutex ready_mtx;

    auto thread_run = [&]()
    {
        std::unique_lock<std::mutex> lock(ready_mtx);
        ready_cv.wait(lock, [&]() -> bool { return ready; });
        lock.unlock();

        for (size_t i = 0; i < num_iterations; ++i) {
            std::unique_lock<SpinMutex> lock(counter_mutex);
            ++counter;
        }
    };

    for (unsigned i = 0; i < num_threads; ++i)
        threads[i] = std::thread(thread_run);

    std::unique_lock<std::mutex> lock(ready_mtx);
    ready = true;
    ready_cv.notify_all();
    lock.unlock();

    for (unsigned i = 0; i < num_threads; ++i)
        threads[i].join();

    REQUIRE(counter == num_threads * num_iterations);
}
