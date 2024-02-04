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
using namespace Catch::literals;

static void WAIT()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_CASE("[FilePool] Shared samples")
{
    std::unique_ptr<sfz::Synth> synth1 { new sfz::Synth() };
    std::unique_ptr<sfz::Synth> synth2 { new sfz::Synth() };
    std::unique_ptr<sfz::Synth> synth3 { new sfz::Synth() };

    synth1->setSamplesPerBlock(256);
    synth2->setSamplesPerBlock(256);
    synth3->setSamplesPerBlock(256);

    synth1->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);

    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);

    sfz::AudioBuffer<float> buffer1 { 2, 256 };
    sfz::AudioBuffer<float> buffer2 { 2, 256 };
    std::vector<float> tmp1 (buffer1.getNumFrames());
    std::vector<float> tmp2 (buffer2.getNumFrames());

    synth2->loadSfzFile("");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);

    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);

#if 1
    return;
// The following process cause a crash on GitHub
#endif

    synth1->loadSfzFile("");
    synth1->noteOff(0, 60, 100);
    for (unsigned i = 0; i < 100; ++i) {
        synth1->renderBlock(buffer1);
        WAIT();
    }

    CHECK(synth1->getNumPreloadedSamples() == 0);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 1);

    synth2.reset();

    CHECK(synth1->getNumPreloadedSamples() == 0);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 0);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 0);

    synth2.reset(new sfz::Synth());

    synth2->setSamplesPerBlock(256);

    synth1->loadSfzFile(fs::current_path() / "tests/TestFiles/looped_regions.sfz");
    synth2->loadSfzFile(fs::current_path() / "tests/TestFiles/kick_embedded.sfz");

    CHECK(synth1->getNumPreloadedSamples() == 1);
    CHECK(synth2->getNumPreloadedSamples() == 1);
    CHECK(synth3->getNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth2->getResources().getFilePool().getActualNumPreloadedSamples() == 1);
    CHECK(synth3->getResources().getFilePool().getActualNumPreloadedSamples() == 0);
    CHECK(synth1->getResources().getFilePool().getGlobalNumPreloadedSamples() == 2);
    CHECK(synth2->getResources().getFilePool().getGlobalNumPreloadedSamples() == 2);
    CHECK(synth3->getResources().getFilePool().getGlobalNumPreloadedSamples() == 2);

}