// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "AudioReader.h"
#include <sndfile.hh>
#include <algorithm>

namespace sfz {

class BasicSndfileReader : public AudioReader {
public:
    explicit BasicSndfileReader(SndfileHandle handle) : handle_(handle) {}
    virtual ~BasicSndfileReader() {}

    int format() const override;
    int64_t frames() const override;
    unsigned channels() const override;
    unsigned sampleRate() const override;
    bool getInstrument(SF_INSTRUMENT* instrument) override;

protected:
    SndfileHandle handle_;
};

int BasicSndfileReader::format() const
{
    return handle_.format();
}

int64_t BasicSndfileReader::frames() const
{
    return handle_.frames();
}

unsigned BasicSndfileReader::channels() const
{
    return handle_.channels();
}

unsigned BasicSndfileReader::sampleRate() const
{
    return handle_.samplerate();
}

bool BasicSndfileReader::getInstrument(SF_INSTRUMENT* instrument)
{
    if (handle_.command(SFC_GET_INSTRUMENT, instrument, sizeof(SF_INSTRUMENT)) == SF_FALSE)
        return false;
    return true;
}

//------------------------------------------------------------------------------

/**
 * @brief Audio file reader in forward direction
 */
class ForwardReader : public BasicSndfileReader {
public:
    explicit ForwardReader(SndfileHandle handle);
    AudioReaderType type() const override;
    size_t readNextBlock(float* buffer, size_t frames) override;
};

ForwardReader::ForwardReader(SndfileHandle handle)
    : BasicSndfileReader(handle)
{
}

AudioReaderType ForwardReader::type() const
{
    return AudioReaderType::Forward;
}

size_t ForwardReader::readNextBlock(float* buffer, size_t frames)
{
    sf_count_t readFrames = handle_.readf(buffer, frames);
    if (frames <= 0)
        return 0;

    return readFrames;
}

//------------------------------------------------------------------------------

template <size_t N, class T = float>
struct AudioFrame {
    T samples[N];
};

/**
 * @brief Reorder a sequence of frames in reverse
 */
static void reverse_frames(float* data, sf_count_t frames, unsigned channels)
{
    switch (channels) {

#define SPECIALIZE_FOR(N)                                           \
        case N:                                                     \
            std::reverse(                                           \
                reinterpret_cast<AudioFrame<N> *>(data),            \
                reinterpret_cast<AudioFrame<N> *>(data) + frames);  \
            break

    SPECIALIZE_FOR(1);
    SPECIALIZE_FOR(2);

    default:
        for (sf_count_t i = 0; i < frames / 2; ++i) {
            sf_count_t j = frames - 1 - i;
            float* frame1 = &data[i * channels];
            float* frame2 = &data[j * channels];
            for (unsigned c = 0; c < channels; ++c)
                std::swap(frame1[c], frame2[c]);
        }
        break;

#undef SPECIALIZE_FOR
    }
}

//------------------------------------------------------------------------------

/**
 * @brief Audio file reader in reverse direction, for fast-seeking formats
 */
class ReverseReader : public BasicSndfileReader {
public:
    explicit ReverseReader(SndfileHandle handle);
    AudioReaderType type() const override;
    size_t readNextBlock(float* buffer, size_t frames) override;

private:
    sf_count_t position_ {};
};

ReverseReader::ReverseReader(SndfileHandle handle)
    : BasicSndfileReader(handle)
{
    position_ = handle.seek(0, SF_SEEK_END);
}

AudioReaderType ReverseReader::type() const
{
    return AudioReaderType::Reverse;
}

size_t ReverseReader::readNextBlock(float* buffer, size_t frames)
{
    sf_count_t position = position_;
    const unsigned channels = handle_.channels();

    const sf_count_t readFrames = std::min<sf_count_t>(frames, position);
    if (readFrames <= 0)
        return false;

    position -= readFrames;
    if (handle_.seek(position, SEEK_SET) != position ||
        handle_.readf(buffer, readFrames) != readFrames)
        return false;

    position_ = position;
    reverse_frames(buffer, readFrames, channels);
    return readFrames;
}

//------------------------------------------------------------------------------

/**
 * @brief Audio file reader in reverse direction, for slow-seeking formats
 */
class NoSeekReverseReader : public BasicSndfileReader {
public:
    explicit NoSeekReverseReader(SndfileHandle handle);
    AudioReaderType type() const override;
    size_t readNextBlock(float* buffer, size_t frames) override;

private:
    void readWholeFile();

private:
    std::unique_ptr<float[]> fileBuffer_;
    sf_count_t fileFramesLeft_ { 0 };
};

NoSeekReverseReader::NoSeekReverseReader(SndfileHandle handle)
    : BasicSndfileReader(handle)
{
}

AudioReaderType NoSeekReverseReader::type() const
{
    return AudioReaderType::NoSeekReverse;
}

size_t NoSeekReverseReader::readNextBlock(float* buffer, size_t frames)
{
    float* fileBuffer = fileBuffer_.get();
    if (!fileBuffer) {
        readWholeFile();
        fileBuffer = fileBuffer_.get();
    }

    const unsigned channels = handle_.channels();
    const sf_count_t fileFramesLeft = fileFramesLeft_;
    sf_count_t readFrames = std::min<sf_count_t>(frames, fileFramesLeft);
    if (readFrames <= 0)
        return 0;

    std::copy(
        &fileBuffer[channels * (fileFramesLeft - readFrames)],
        &fileBuffer[channels * fileFramesLeft], buffer);
    reverse_frames(buffer, readFrames, channels);

    fileFramesLeft_ = fileFramesLeft - readFrames;
    return readFrames;
}

void NoSeekReverseReader::readWholeFile()
{
    const sf_count_t frames = handle_.frames();
    const unsigned channels = handle_.channels();
    float* fileBuffer = new float[channels * frames];
    fileBuffer_.reset(fileBuffer);
    fileFramesLeft_ = handle_.readf(fileBuffer, frames);
}

//------------------------------------------------------------------------------

const std::error_category& sndfile_category()
{
    class sndfile_category : public std::error_category {
    public:
        const char* name() const noexcept override
        {
            return "sndfile";
        }

        std::string message(int condition) const override
        {
            const char* str = sf_error_number(condition);
            return str ? str : "";
        }
    };

    static const sndfile_category cat;
    return cat;
}

//------------------------------------------------------------------------------

class DummyAudioReader : public AudioReader {
public:
    explicit DummyAudioReader(AudioReaderType type) : type_(type) {}
    AudioReaderType type() const override { return type_; }
    int format() const override { return 0; }
    int64_t frames() const override { return 0; }
    unsigned channels() const override { return 1; }
    unsigned sampleRate() const override { return 44100; }
    size_t readNextBlock(float*, size_t) override { return 0; }
    bool getInstrument(SF_INSTRUMENT* ) override { return false; }

private:
    AudioReaderType type_ {};
};

//------------------------------------------------------------------------------

static bool formatHasFastSeeking(int format)
{
    bool fast;

    const int type = format & SF_FORMAT_TYPEMASK;
    const int subtype = format & SF_FORMAT_SUBMASK;

    switch (type) {
    case SF_FORMAT_WAV:
    case SF_FORMAT_AIFF:
    case SF_FORMAT_AU:
    case SF_FORMAT_RAW:
    case SF_FORMAT_WAVEX:
        // TODO: list more PCM formats that support fast seeking
        fast = subtype >= SF_FORMAT_PCM_S8 && subtype <= SF_FORMAT_DOUBLE;
        break;
    case SF_FORMAT_FLAC:
        // seeking has acceptable overhead
        fast = true;
        break;
    case SF_FORMAT_OGG:
        // ogg is prohibitively slow at seeking (possibly others)
        // cf. https://github.com/erikd/libsndfile/issues/491
        fast = false;
        break;
    default:
        fast = false;
        break;
    }

    return fast;
}

AudioReaderPtr createAudioReader(const fs::path& path, bool reverse, std::error_code* ec)
{
    AudioReaderPtr reader;

    if (ec)
        ec->clear();

#if defined(_WIN32)
    SndfileHandle handle(path.wstring().c_str());
#else
    SndfileHandle handle(path.c_str());
#endif

    if (!handle) {
        if (ec)
            *ec = std::error_code(handle.error(), sndfile_category());
        reader.reset(new DummyAudioReader(reverse ? AudioReaderType::Reverse : AudioReaderType::Forward));
    }
    else if (!reverse)
        reader.reset(new ForwardReader(handle));
    else if (formatHasFastSeeking(handle.format()))
        reader.reset(new ReverseReader(handle));
    else
        reader.reset(new NoSeekReverseReader(handle));

    return reader;
}

AudioReaderPtr createExplicitAudioReader(const fs::path& path, AudioReaderType type, std::error_code* ec)
{
    AudioReaderPtr reader;

    if (ec)
        ec->clear();

#if defined(_WIN32)
    SndfileHandle handle(path.wstring().c_str());
#else
    SndfileHandle handle(path.c_str());
#endif

    if (!handle) {
        if (ec)
            *ec = std::error_code(handle.error(), sndfile_category());
        reader.reset(new DummyAudioReader(type));
    }
    else {
        switch (type) {
        case AudioReaderType::Forward:
            reader.reset(new ForwardReader(handle));
            break;
        case AudioReaderType::Reverse:
            reader.reset(new ReverseReader(handle));
            break;
        case AudioReaderType::NoSeekReverse:
            reader.reset(new NoSeekReverseReader(handle));
            break;
        }
    }

    return reader;
}

} // namespace sfz
