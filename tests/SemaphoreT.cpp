// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/RTSemaphore.h"
#include "catch2/catch.hpp"
#include <thread>

TEST_CASE("[Semaphore] Basic operations")
{
    RTSemaphore sem;

    REQUIRE(sem.try_wait() == false);

    sem.post();
    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == false);

    sem.post();
    sem.post();
    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == false);

    sem.post();
    sem.post();
    sem.wait();
    sem.wait();

    REQUIRE(sem.try_wait() == false);
}

TEST_CASE("[Semaphore] Counter initialization")
{
    RTSemaphore sem(3);

    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == true);
    REQUIRE(sem.try_wait() == false);
}

TEST_CASE("[Semaphore] Thread synchronization")
{
    RTSemaphore sem1;
    RTSemaphore sem2;
    constexpr int n = 1000;

    std::thread t1([&]() {
        for (int i = 0; i < n; ++i) {
            sem1.post();
            sem2.wait();
        }
    });
    std::thread t2([&]() {
        for (int i = 0; i < n; ++i) {
            sem2.post();
            sem1.wait();
        }
    });

    t1.join();
    t2.join();
    REQUIRE(sem1.try_wait() == false);
    REQUIRE(sem2.try_wait() == false);
}
