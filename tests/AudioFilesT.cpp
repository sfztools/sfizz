// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioSpan.h"
#include "absl/types/span.h"
#include "sfizz/Synth.h"
#include "TestHelpers.h"
#include "catch2/catch.hpp"
#include "st_audiofile.hpp"
#include <algorithm>
#include <vector>
using namespace Catch::literals;

void compareFiles(const fs::path& lFile, const fs::path& rFile)
{
    auto l_file = ST_AudioFile();
    auto r_file = ST_AudioFile();
#if defined(_WIN32)
    l_file.open_file_w(lFile.wstring().c_str());
    r_file.open_file_w(rFile.wstring().c_str());
#else
    l_file.open_file(lFile.string().c_str());
    r_file.open_file(rFile.string().c_str());
#endif
    CHECK(l_file.get_channels() == 1);
    CHECK(r_file.get_channels() == 1);
    std::vector<float> wav (l_file.get_frame_count());
    std::vector<float> flac (r_file.get_frame_count());
    l_file.read_f32(wav.data(), l_file.get_frame_count());
    r_file.read_f32(flac.data(), r_file.get_frame_count());
    REQUIRE( approxEqual<float>(wav, flac) );
}

TEST_CASE("[AudioFiles] Compare Flac and WAV")
{
    compareFiles(fs::current_path() / "tests/TestFiles/kick.wav",
        fs::current_path() / "tests/TestFiles/kick.flac");
}

#if !defined(SFIZZ_USE_SNDFILE)
TEST_CASE("[AudioFiles] Compare WV and WAV")
{
    compareFiles(fs::current_path() / "tests/TestFiles/kick.wav",
        fs::current_path() / "tests/TestFiles/kick.wv");
}
#endif

struct CompareOutputOpts
{
    int note { 60 };
    int delay { 0 };
    int velocity { 127 };
    float sampleRate { 48000.0f };
    int samplesPerBlock { 1024 };
};

/**
 * @brief Compare the outputs of 2 sfz files with a given set of options, for a single note pressed.
 *
 * @param lFile
 * @param rFile
 * @param opts
 */
void compareOutputs(const std::string& lFile, const std::string& rFile, CompareOutputOpts opts)
{
    sfz::Synth lSynth;
    sfz::Synth rSynth;
    lSynth.enableFreeWheeling();
    rSynth.enableFreeWheeling();
    lSynth.setSampleRate(opts.sampleRate);
    rSynth.setSampleRate(opts.sampleRate);
    lSynth.setSamplesPerBlock(opts.samplesPerBlock);
    rSynth.setSamplesPerBlock(opts.samplesPerBlock);
    sfz::AudioBuffer<float> lBuffer { 2, static_cast<unsigned>(lSynth.getSamplesPerBlock()) };
    sfz::AudioBuffer<float> rBuffer { 2, static_cast<unsigned>(rSynth.getSamplesPerBlock()) };
    sfz::AudioSpan<float> lSpan { lBuffer };
    sfz::AudioSpan<float> rSpan { rBuffer };
    lSynth.loadSfzString(fs::current_path() / "tests/TestFiles/l.sfz", lFile);
    rSynth.loadSfzString(fs::current_path() / "tests/TestFiles/r.sfz", rFile);
    lSynth.noteOn(opts.delay, opts.note, opts.velocity);
    rSynth.noteOn(opts.delay, opts.note, opts.velocity);
    lSynth.renderBlock(lSpan);
    rSynth.renderBlock(rSpan);
    REQUIRE( numPlayingVoices(lSynth) == 1 );
    REQUIRE( numPlayingVoices(rSynth) == 1 );
    REQUIRE( approxEqual(lSpan.getConstSpan(0), rSpan.getConstSpan(0)) );
    REQUIRE( approxEqual(lSpan.getConstSpan(1), rSpan.getConstSpan(1)) );
    while (numPlayingVoices(lSynth) == 1) {
        REQUIRE( numPlayingVoices(rSynth) == 1 );
        lSynth.renderBlock(lSpan);
        rSynth.renderBlock(rSpan);
        REQUIRE( approxEqual(lSpan.getConstSpan(0), rSpan.getConstSpan(0)) );
        REQUIRE( approxEqual(lSpan.getConstSpan(1), rSpan.getConstSpan(1)) );
    }
}

// FIXME: these fail somewhat randomly

#if 0
TEST_CASE("[AudioFiles] Sanity check (native sample rate)")
{
    std::string lFile = "<region> sample=kick.wav key=60";
    std::string rFile = "<region> sample=kick.wav key=60";
    CompareOutputOpts opts;
    opts.sampleRate = 44100.0f;
    compareOutputs(lFile, rFile, opts);
}

#if !defined(SFIZZ_USE_SNDFILE)
TEST_CASE("[AudioFiles] Wavpack file (native sample rate)")
{
    std::string lFile = "<region> sample=kick.wav key=60";
    std::string rFile = "<region> sample=kick.wv key=60";
    CompareOutputOpts opts;
    opts.sampleRate = 44100.0f;
    compareOutputs(lFile, rFile, opts);
}

TEST_CASE("[AudioFiles] Wavpack file (resampled)")
{
    std::string lFile = "<region> sample=kick.wav key=60";
    std::string rFile = "<region> sample=kick.wv key=60";
    CompareOutputOpts opts;
    opts.sampleRate = 48000.0f;
    compareOutputs(lFile, rFile, opts);
}
#endif

TEST_CASE("[AudioFiles] Flac file (native sample rate)")
{
    std::string lFile = "<region> sample=kick.wav key=60";
    std::string rFile = "<region> sample=kick.flac key=60";
    CompareOutputOpts opts;
    opts.sampleRate = 44100.0f;
    compareOutputs(lFile, rFile, opts);
}

TEST_CASE("[AudioFiles] Flac file (resampled)")
{
    std::string lFile = "<region> sample=kick.wav key=60";
    std::string rFile = "<region> sample=kick.flac key=60";
    CompareOutputOpts opts;
    opts.sampleRate = 48000.0f;
    compareOutputs(lFile, rFile, opts);
}
#endif

TEST_CASE("[Files] Embedded sample data")
{
    sfz::Synth synth1;
    sfz::Synth synth2;

    synth1.enableFreeWheeling();
    synth2.enableFreeWheeling();

    synth1.setSamplesPerBlock(256);
    synth2.setSamplesPerBlock(256);

    synth1.loadSfzFile(fs::current_path() / "tests/TestFiles/kick.sfz");
    synth2.loadSfzFile(fs::current_path() / "tests/TestFiles/kick_embedded.sfz");

    REQUIRE(synth1.getNumPreloadedSamples() == 1);
    REQUIRE(synth2.getNumPreloadedSamples() == 1);

    sfz::AudioBuffer<float> buffer1 { 2, 256 };
    sfz::AudioBuffer<float> buffer2 { 2, 256 };
    std::vector<float> tmp1 (buffer1.getNumFrames());
    std::vector<float> tmp2 (buffer2.getNumFrames());

    synth1.noteOn(0, 60, 100);
    synth2.noteOn(0, 60, 100);

    for (unsigned i = 0; i < 100; ++i) {
        synth1.renderBlock(buffer1);
        synth2.renderBlock(buffer2);

        const auto left1 = buffer1.getConstSpan(0);
        const auto left2 = buffer2.getConstSpan(0);
        std::copy(left1.begin(), left1.end(), tmp1.begin());
        std::copy(left2.begin(), left2.end(), tmp2.begin());
        REQUIRE(tmp1 == tmp2);

        const auto right1 = buffer1.getConstSpan(0);
        const auto right2 = buffer2.getConstSpan(0);
        std::copy(right1.begin(), right1.end(), tmp1.begin());
        std::copy(right2.begin(), right2.end(), tmp2.begin());
        REQUIRE(tmp1 == tmp2);
    }
}
