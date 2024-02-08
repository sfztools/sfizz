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

static void WAIT(int ms)
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
            WAIT(200);
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
        WAIT(10);
    }

    CHECK(synth1->getNumPreloadedSamples() == 0);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    synth2.reset();
    WAIT(1100);

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
        WAIT(10);
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
        WAIT(10);
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
        WAIT(10);
    }
    WAIT(1100);

    CHECK(synth1->getNumPreloadedSamples() == 4);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 4);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 5);

    // Release
    synth3.reset();
    WAIT(1100);
    CHECK(synth1->getNumPreloadedSamples() == 4);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 5);

    synth1.reset();
    synth2.reset();
    WAIT(1100);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    filePoolGlobalObj.reset();
    filePoolGlobalObj = sfz::FilePool::getGlobalObject();
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);
}

// It crashes strangely if this value is a large number.
// When I run "sfizz_tests" directly, it doesn't crash.
// static const unsigned synthCount = 100;
static const unsigned synthCount = 10;

TEST_CASE("[FilePool] Stress Test")
{
    struct TestSynthThread {
        TestSynthThread()
        {
            synth.setSamplesPerBlock(256);
        }

        ~TestSynthThread()
        {
            running = false;
            std::error_code ec;
            semBarrier.post(ec);
            ASSERT(!ec);
            thread.join();
        }

        void job() noexcept
        {
            semBarrier.wait();
            synth.loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");
            if (execution)
                execution();
            if (running) {
                while (semBarrier.wait(), running) {
                    synth.renderBlock(buffer);
                    if (execution)
                        execution();
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
        std::thread thread { &TestSynthThread::job, this };
        RTSemaphore semBarrier { 0 };
        bool running { true };
        std::function<void()> execution;
    };

    std::unique_ptr<TestSynthThread[]> synthThreads { new TestSynthThread[synthCount]() };
    std::shared_ptr<sfz::FilePool::GlobalObject> filePoolGlobalObj { sfz::FilePool::getGlobalObject() };
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);

    std::atomic<int> finishCount { 0 };
    auto countFunc = [&finishCount]() {
        ++finishCount;
    };
    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i].execution = countFunc;
    }

    finishCount = 0;
    for (unsigned j = 0; j < synthCount; ++j) {
        synthThreads[j].trigger();
    }
    int count = 0;
    while (finishCount != synthCount) {
        CHECK(++count < 65536);
        WAIT(100);
    }
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 1);

    for (unsigned i = 0; i < synthCount; ++i) {
        CHECK(synthThreads[i].synth.getNumPreloadedSamples() == 1);
    }
    
    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i].noteOn();
    }
    for (unsigned i = 0; i < 100; ++i) {
        finishCount = 0;
        for (unsigned j = 0; j < synthCount; ++j) {
            synthThreads[j].trigger();
        }
        while (finishCount != synthCount) {
            WAIT(10);
        }
    }

    for (unsigned i = 0; i < synthCount; ++i) {
        synthThreads[i].noteOff();
    }
    for (unsigned i = 0; i < 100; ++i) {
        finishCount = 0;
        for (unsigned j = 0; j < synthCount; ++j) {
            synthThreads[j].trigger();
        }
        while (finishCount != synthCount) {
            WAIT(10);
        }
    }

    std::unique_ptr<sfz::Synth> synth1 { new sfz::Synth() };
    synthThreads.reset();
    WAIT(1100);
    CHECK(filePoolGlobalObj->getNumLoadedSamples() == 0);
}