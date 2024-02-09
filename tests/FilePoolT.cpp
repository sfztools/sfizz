// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioSpan.h"
#include "absl/types/span.h"
#include "sfizz/Synth.h"
#include "sfizz/FilePool.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>

using namespace Catch::literals;

static void wait_ms(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

TEST_CASE("[FilePool] Ready Mutex")
{
    struct ProcessData
    {
        bool dummy;
        sfz::FileData fileData {};
        sfz::FilePool filePool {dummy};
        void prepare()
        {
            wait_ms(200);
            fileData.initWith(sfz::FileData::Status::Preloaded, sfz::FileData());
        }
        void wait()
        {
            fileData.addSecondaryOwner(&filePool);
        }
    };
    ProcessData p;
    std::thread t1([&p] { p.wait(); });
    std::thread t2([&p] { p.prepare(); });
    t1.join();
    t2.join();
    ASSERT(1);
}

TEST_CASE("[FilePool] Shared samples")
{
    std::unique_ptr<sfz::Synth> synth1 { new sfz::Synth() };
    std::unique_ptr<sfz::Synth> synth2 { new sfz::Synth() };
    std::unique_ptr<sfz::Synth> synth3 { new sfz::Synth() };
    std::shared_ptr<sfz::FilePool::GlobalObject> filePoolGlobalObj { sfz::FilePool::getGlobalObject() };

    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    synth1->setSamplesPerBlock(256);
    synth2->setSamplesPerBlock(256);
    synth3->setSamplesPerBlock(256);

    synth1->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    sfz::AudioBuffer<float> buffer { 2, 256 };

    synth2->loadSfzFile("");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    synth1->loadSfzFile("");
    for (unsigned i = 0; i < 100; ++i) {
        synth1->renderBlock(buffer);
        synth2->renderBlock(buffer);
        synth3->renderBlock(buffer);
        wait_ms(10);
    }

    CHECK(synth1->getNumPreloadedSamples() == 0);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    synth2.reset();
    wait_ms(1100);

    CHECK(synth1->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    synth2.reset(new sfz::Synth());
    synth2->setSamplesPerBlock(256);

    synth1->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");
    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/kick_embedded.sfz");

    for (unsigned i = 0; i < 100; ++i) {
        synth1->renderBlock(buffer);
        synth2->renderBlock(buffer);
        synth3->renderBlock(buffer);
        wait_ms(10);
    }
    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 2);

    synth3->loadSfzFile(fs::current_path() / "tests/TestFiles/wavetables.sfz");

    for (unsigned i = 0; i < 100; ++i) {
        synth1->renderBlock(buffer);
        synth2->renderBlock(buffer);
        synth3->renderBlock(buffer);
        wait_ms(10);
    }
    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 4);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 6);

    synth1->loadSfzFile(fs::current_path() / "tests/TestFiles/wavetables.sfz");
    for (unsigned i = 0; i < 100; ++i) {
        synth1->renderBlock(buffer);
        synth2->renderBlock(buffer);
        synth3->renderBlock(buffer);
        wait_ms(10);
    }
    wait_ms(1100);

    CHECK(synth1->getNumPreloadedSamples() == 4);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 4);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 5);

    // Release
    synth3.reset();
    wait_ms(1100);
    CHECK(synth1->getNumPreloadedSamples() == 4);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 5);

    synth1.reset();
    synth2.reset();
    wait_ms(1100);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    filePoolGlobalObj.reset();
    filePoolGlobalObj = sfz::FilePool::getGlobalObject();
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);
}

static const int synthCount = 100;

TEST_CASE("[FilePool] Stress Test")
{
    struct TestSynthThread {
        enum Status {
            Initialized,
            Waiting1,
            Loading,
            Waiting2,
            Rendering,
            Posted,
        };

        TestSynthThread()
        {
            CHECK((bool)semBarrier);
            synth.setSamplesPerBlock(256);
            thread.reset(new std::thread( &TestSynthThread::job, this ));
        }

        ~TestSynthThread()
        {
            running.store(false, std::memory_order_release);
            flag.store(false, std::memory_order_release);
            status.store(Status::Posted, std::memory_order_release);
            std::error_code ec;
            semBarrier.post(ec);
            CHECK(!ec);
            thread->join();
        }

        void job()
        {
            status.store(Status::Waiting1, std::memory_order_release);
            semBarrier.wait();
            CHECK(status.load(std::memory_order_acquire) == Status::Posted);
            CHECK(flag.load(std::memory_order_acquire) == false);
            flag.store(true, std::memory_order_release);
            status.store(Status::Loading, std::memory_order_release);
            synth.loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");
            if (running.load(std::memory_order_acquire)) {
                status.store(Status::Waiting2, std::memory_order_release);
                while (semBarrier.wait(), running.load(std::memory_order_acquire)) {
                    CHECK(status.load(std::memory_order_acquire) == Status::Posted);
                    CHECK(!flag.load(std::memory_order_acquire));
                    flag.store(true, std::memory_order_release);
                    status.store(Status::Rendering, std::memory_order_release);
                    synth.renderBlock(buffer);
                    status.store(Status::Waiting2, std::memory_order_release);
                }
            }
        }

        void noteOn()
        {
            synth.noteOn(0, 60, 100);
        }

        void noteOff()
        {
            synth.noteOff(0, 60, 100);
        }

        void trigger()
        {
            std::error_code ec;
            semBarrier.post(ec);
            ASSERT(!ec);
        }

        sfz::AudioBuffer<float> buffer { 2, 256 };
        sfz::Synth synth {};
        std::unique_ptr<std::thread> thread {};
        RTSemaphore semBarrier { 0 };
        std::atomic<bool> running { true };
        std::atomic<Status> status { Status::Initialized };
        std::atomic<bool> flag { true };
    };

    std::unique_ptr<TestSynthThread> synthThreads[synthCount];
    for (int i = 0; i < synthCount; ++i) {
        synthThreads[i].reset(new TestSynthThread());
    }
    wait_ms(100);
    std::shared_ptr<sfz::FilePool::GlobalObject> filePoolGlobalObj { sfz::FilePool::getGlobalObject() };
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    auto waitForStatus = [&](int max_counter, TestSynthThread::Status statusToWait) {
        for (int j = 0; j < max_counter; ++j) {
            int flagCount = 0;
            for (int i = 0; i < synthCount; ++i) {
                if (synthThreads[i]->status.load(std::memory_order_acquire) == statusToWait) {
                    flagCount++;
                }
                else {
                    wait_ms(10);
                }
            }
            if (flagCount == synthCount) {
                wait_ms(10);
                for (int i = 0; i < synthCount; ++i) {
                    ASSERT(synthThreads[i]->status.load(std::memory_order_acquire) == statusToWait);
                }
                return true;
            }
        }
        return false;
    };

    auto triggerFunc = [&]() {
        for (int i = 0; i < synthCount; ++i) {
            synthThreads[i]->flag.store(false, std::memory_order_release);
        }
        for (int i = 0; i < synthCount; ++i) {
            ASSERT(synthThreads[i]->flag.load(std::memory_order_acquire) == false);
            synthThreads[i]->status.store(TestSynthThread::Status::Posted, std::memory_order_release);
            synthThreads[i]->trigger();
        }
    };

    CHECK(waitForStatus(200000, TestSynthThread::Status::Waiting1));
    triggerFunc();
    CHECK(waitForStatus(1000, TestSynthThread::Status::Waiting2));

    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);
    for (unsigned i = 0; i < synthCount; ++i) {
        CHECK(synthThreads[i]->synth.getNumPreloadedSamples() == 1);
    }
    
    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i]->noteOn();
    }
    for (unsigned i = 0; i < 100; ++i) {
        triggerFunc();
        CHECK(waitForStatus(10, TestSynthThread::Status::Waiting2));
    }

    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i]->noteOff();
    }
    for (unsigned i = 0; i < 100; ++i) {
        triggerFunc();
        CHECK(waitForStatus(10, TestSynthThread::Status::Waiting2));
    }
    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i].reset();
    }
    wait_ms(1100);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);
}