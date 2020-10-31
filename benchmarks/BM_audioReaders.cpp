// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioReader.h"
#include <benchmark/benchmark.h>
#include <sndfile.hh>
#include <ghc/fs_std.hpp>
#include <vector>
#include <memory>
#include <system_error>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#else
#include <io.h>
#endif

///
struct TemporaryFile {
    TemporaryFile();
    ~TemporaryFile();

    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;

    TemporaryFile(TemporaryFile&&) = default;
    TemporaryFile& operator=(TemporaryFile&&) = default;

    const fs::path& path() const { return path_; }

private:
    fs::path path_;
    static fs::path createTemporaryFile();
};

TemporaryFile::TemporaryFile()
    : path_(createTemporaryFile())
{
}

TemporaryFile::~TemporaryFile()
{
    std::error_code ec;
    fs::remove(path_, ec);
}

#if !defined(_WIN32)
fs::path TemporaryFile::createTemporaryFile()
{
    char path[] = P_tmpdir "/sndXXXXXX";
    int fd = mkstemp(path);
    if (fd == -1)
        throw std::runtime_error("Cannot create temporary file.");
    close(fd);
    return path;
}
#else
fs::path TemporaryFile::createTemporaryFile()
{
    DWORD ret = GetTempPathW(0, buffer);
    std::unique_ptr<WCHAR[]> path;
    if (ret != 0) {
        path.reset(new WCHAR[ret + 8]{});
        ret = GetTempPathW(0, buffer);
        if (ret != 0)
            wcscat(path.get(), L"\\XXXXXX");
    }
    if (ret == 0 || !_wmktemp(path.get()))
        throw std::runtime_error("Cannot create temporary file.");
    return path.get();
}
#endif

///
class AudioReaderFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override
    {
        workBuffer.resize(2 * static_cast<size_t>(state.range(0)));
    }

    void TearDown(const ::benchmark::State& /* state */) override
    {
    }

    static TemporaryFile createAudioFile(int format);

    static TemporaryFile fileWav;
    static TemporaryFile fileFlac;
    static TemporaryFile fileAiff;
    static TemporaryFile fileOgg;

    std::vector<float> workBuffer;
};

TemporaryFile AudioReaderFixture::fileWav = createAudioFile(SF_FORMAT_WAV|SF_FORMAT_PCM_16);
TemporaryFile AudioReaderFixture::fileFlac = createAudioFile(SF_FORMAT_FLAC|SF_FORMAT_PCM_16);
TemporaryFile AudioReaderFixture::fileAiff = createAudioFile(SF_FORMAT_AIFF|SF_FORMAT_PCM_16);
TemporaryFile AudioReaderFixture::fileOgg = createAudioFile(SF_FORMAT_OGG|SF_FORMAT_VORBIS);

TemporaryFile AudioReaderFixture::createAudioFile(int format)
{
    constexpr unsigned sampleRate = 44100;
    constexpr unsigned fileDuration = 10;
    constexpr unsigned fileFrames = sampleRate * fileDuration;

    // synth 2 channels of arbitrary waveform
    std::unique_ptr<double[]> sndData { new double[2 * fileFrames] };
    double phase = 0.0;
    for (unsigned i = 0; i < fileFrames; ++i) {
        sndData[2 * i    ] = std::sin(2.0 * M_PI * phase);
        sndData[2 * i + 1] = std::cos(2.0 * M_PI * phase);
        phase += 440.0 * (1.0 / sampleRate);
        phase -= static_cast<long>(phase);
    }

    // create temp file
    TemporaryFile temp;
    fprintf(stderr, "* Temporary file: %s\n", temp.path().u8string().c_str());

    // write to file
#if !defined(_WIN32)
    SndfileHandle snd(temp.path().c_str(), SFM_WRITE, format, 2, sampleRate);
#else
    SndfileHandle snd(temp.path().wstring().c_str(), SFM_WRITE, format, 2, sampleRate);
#endif

    if (snd.error())
        throw std::runtime_error("cannot open sound file for writing");
    snd.writef(sndData.get(), fileFrames);
    snd = SndfileHandle();

    return temp;
}

static void doReaderBenchmark(const fs::path& path, std::vector<float> &buffer, sfz::AudioReaderType type)
{
    sfz::AudioReaderPtr reader = sfz::createExplicitAudioReader(path, type);
    while (reader->readNextBlock(buffer.data(), buffer.size() / 2) > 0);
}

static void doEntireRead(const fs::path& path)
{
    sfz::AudioReaderPtr reader = sfz::createAudioReader(path, false);
    if (!reader)
        return;

    std::vector<float> buffer(static_cast<size_t>(2 * reader->frames()));
    reader->readNextBlock(buffer.data(), buffer.size());
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileWav.path());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileWav.path(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileWav.path(), workBuffer, sfz::AudioReaderType::Reverse);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileFlac.path());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileFlac.path(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileFlac.path(), workBuffer, sfz::AudioReaderType::Reverse);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireAiff)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileAiff.path());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardAiff)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileAiff.path(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseAiff)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileAiff.path(), workBuffer, sfz::AudioReaderType::Reverse);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireOgg)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileOgg.path());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardOgg)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileOgg.path(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

#if !defined(ST_AUDIO_FILE_USE_SNDFILE)
BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseOgg)(benchmark::State& state)
{
   for (auto _ : state) {
       doReaderBenchmark(fileOgg.path(), workBuffer, sfz::AudioReaderType::Reverse);
   }
}
#endif

BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardWav)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseWav)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireWav)->Range(1, 1);
BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardFlac)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseFlac)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireFlac)->Range(1, 1);
BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardAiff)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseAiff)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireAiff)->Range(1, 1);
BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardOgg)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
#if !defined(ST_AUDIO_FILE_USE_SNDFILE)
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseOgg)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
#endif
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireOgg)->Range(1, 1);
BENCHMARK_MAIN();
