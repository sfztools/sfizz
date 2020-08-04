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
#include <cmath>
#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#else
#include <io.h>
#endif

///
struct AutoFD {
    AutoFD() {}
    ~AutoFD() { reset(); }

    AutoFD(const AutoFD&) = delete;
    AutoFD &operator=(const AutoFD&) = delete;

    AutoFD(AutoFD&& other) : fd_(other.fd_) { other.fd_ = -1; }
    AutoFD &operator=(AutoFD&& other)
    {
        if (this == &other) return *this;
        reset(other.fd_);
        other.fd_ = -1;
        return *this;
    }

    explicit operator bool() const noexcept { return fd_ != -1; }
    int get() const noexcept { return fd_; }

    int release()
    {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

    void reset(int fd = -1) noexcept
    {
        if (fd_ == fd) return;
        if (fd_ != -1) close(fd_);
        fd_ = fd;
    }

private:
    int fd_ = -1;
};

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

    static AutoFD createAudioFile(int format);

    static AutoFD fileWav;
    static AutoFD fileFlac;
    static AutoFD fileOgg;

    std::vector<float> workBuffer;
};

AutoFD AudioReaderFixture::fileWav = createAudioFile(SF_FORMAT_WAV|SF_FORMAT_PCM_16);
AutoFD AudioReaderFixture::fileFlac = createAudioFile(SF_FORMAT_FLAC|SF_FORMAT_PCM_16);
AutoFD AudioReaderFixture::fileOgg = createAudioFile(SF_FORMAT_OGG|SF_FORMAT_VORBIS);

AutoFD AudioReaderFixture::createAudioFile(int format)
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

    // create anonymous temp file
    FILE* file = tmpfile();
    if (!file)
        throw std::system_error(errno, std::generic_category());

    // convert FILE to fd, for sndfile
    AutoFD fd;
    fd.reset(dup(fileno(file)));
    if (!fd) {
        fclose(file);
        throw std::system_error(errno, std::generic_category());
    }
    fclose(file);

    // write to fd
    SndfileHandle snd(fd.get(), false, SFM_WRITE, format, 2, sampleRate);
    if (snd.error())
        throw std::runtime_error("cannot open sound file for writing");
    snd.writef(sndData.get(), fileFrames);
    snd = SndfileHandle();

    return fd;
}

static void rewindFd(int fd)
{
#ifndef _WIN32
    off_t off = lseek(fd, 0, SEEK_SET);
#else
    off_t off = _lseek(fd, 0, SEEK_SET);
#endif
    if (off == -1)
        throw std::system_error(errno, std::generic_category());
}

static void doReaderBenchmark(int fd, std::vector<float> &buffer, sfz::AudioReaderType type)
{
    rewindFd(fd);
    sfz::AudioReaderPtr reader = sfz::createExplicitAudioReaderWithFd(fd, type);
    while (reader->readNextBlock(buffer.data(), buffer.size() / 2) > 0);
}

static void doEntireRead(int fd)
{
    rewindFd(fd);

    SndfileHandle handle(fd, false);
    if (handle.error())
        throw std::runtime_error("cannot open sound file for reading");

    std::vector<float> buffer(static_cast<size_t>(2 * handle.frames()));
    handle.read(buffer.data(), buffer.size());
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileWav.get());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileWav.get(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseWav)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileWav.get(), workBuffer, sfz::AudioReaderType::Reverse);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileFlac.get());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileFlac.get(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseFlac)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileFlac.get(), workBuffer, sfz::AudioReaderType::Reverse);
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, EntireOgg)(benchmark::State& state)
{
    for (auto _ : state) {
        doEntireRead(fileOgg.get());
    }
}

BENCHMARK_DEFINE_F(AudioReaderFixture, ForwardOgg)(benchmark::State& state)
{
    for (auto _ : state) {
        doReaderBenchmark(fileOgg.get(), workBuffer, sfz::AudioReaderType::Forward);
    }
}

//BENCHMARK_DEFINE_F(AudioReaderFixture, ReverseOgg)(benchmark::State& state)
//{
//    for (auto _ : state) {
//        doReaderBenchmark(fileOgg.get(), workBuffer, sfz::AudioReaderType::Reverse);
//    }
//}

BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardWav)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseWav)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireWav)->Range(1, 1);
BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardFlac)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseFlac)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireFlac)->Range(1, 1);
BENCHMARK_REGISTER_F(AudioReaderFixture, ForwardOgg)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
//BENCHMARK_REGISTER_F(AudioReaderFixture, ReverseOgg)->RangeMultiplier(2)->Range((1 << 6), (1 << 10));
BENCHMARK_REGISTER_F(AudioReaderFixture, EntireOgg)->Range(1, 1);
BENCHMARK_MAIN();
