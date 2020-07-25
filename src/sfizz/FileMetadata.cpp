// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileMetadata.h"
#include <cstdio>
#include <cstring>

namespace sfz {

// Utility: file cleanup

struct FILE_deleter {
    void operator()(FILE* x) const noexcept { fclose(x); }
};
typedef std::unique_ptr<FILE, FILE_deleter> FILE_u;

// Utility: binary file IO

static bool fread_u32le(FILE* stream, uint32_t& value)
{
    uint8_t bytes[4];
    if (fread(bytes, 4, 1, stream) != 1)
        return false;
    value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    return true;
}

static bool fread_u32be(FILE* stream, uint32_t& value)
{
    uint8_t bytes[4];
    if (fread(bytes, 4, 1, stream) != 1)
        return false;
    value = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return true;
}

//------------------------------------------------------------------------------

struct FileMetadataReader::Impl {
    FILE_u stream_;
    std::vector<RiffChunkInfo> riffChunks_;

    bool openFlac();
    bool openRiff();
};

FileMetadataReader::FileMetadataReader()
    : impl_(new Impl)
{
    impl_->riffChunks_.reserve(16);
}

FileMetadataReader::~FileMetadataReader()
{
}

bool FileMetadataReader::open(const fs::path& path)
{
    close();

#if !defined(_WIN32)
    FILE* stream = fopen(path.c_str(), "rb");
#else
    FILE* stream = _wfopen(path.wstring().c_str(), L"rb");
#endif

    if (!stream)
        return false;

    impl_->stream_.reset(stream);

    char magic[4];
    size_t count = fread(magic, 1, sizeof(magic), stream);

    if (count >= 4 && !memcmp(magic, "fLaC", 4)) {
        if (!impl_->openFlac()) {
            close();
            return false;
        }
    }
    else if (count >= 4 && !memcmp(magic, "RIFF", 4)) {
        if (!impl_->openRiff()) {
            close();
            return false;
        }
    }

    return true;
}

void FileMetadataReader::close()
{
    impl_->stream_.reset();
    impl_->riffChunks_.clear();
}

bool FileMetadataReader::Impl::openFlac()
{
    FILE* stream = stream_.get();
    std::vector<RiffChunkInfo>& riffChunks = riffChunks_;

    if (fseek(stream, 4, SEEK_SET) != 0)
        return false;

    uint32_t header = 0;
    while (((header >> 31) & 1) != 1) {
        if (!fread_u32be(stream, header))
            return false;

        const uint32_t blockType = (header >> 24) & 0x7f;
        const uint32_t blockSize = header & ((1 << 24) - 1);

        const off_t offStartBlock = ftell(stream);
        const off_t offNextBlock = offStartBlock + blockSize;

        if (blockType == 2) { // APPLICATION block
            char blockId[4];
            char riffId[4];
            uint32_t riffChunkSize;
            if (fread(blockId, 4, 1, stream) == 1 && memcmp(blockId, "riff", 4) == 0 &&
                fread(riffId, 4, 1, stream) == 1 &&
                fread_u32le(stream, riffChunkSize) && riffChunkSize <= blockSize - 12)
            {
                RiffChunkInfo info;
                info.index = riffChunks.size();
                info.fileOffset = ftell(stream);
                memcpy(info.id.data(), riffId, 4);
                info.length = riffChunkSize;
                riffChunks.push_back(info);
            }
        }

        if (fseek(stream, offNextBlock, SEEK_SET) != 0)
            return false;
    }

    return true;
}

bool FileMetadataReader::Impl::openRiff()
{
    FILE* stream = stream_.get();
    std::vector<RiffChunkInfo>& riffChunks = riffChunks_;

    if (fseek(stream, 12, SEEK_SET) != 0)
        return false;

    char riffId[4];
    uint32_t riffChunkSize;
    while (fread(riffId, 4, 1, stream) == 1 && fread_u32le(stream, riffChunkSize)) {
        RiffChunkInfo info;
        info.index = riffChunks.size();
        info.fileOffset = ftell(stream);
        memcpy(info.id.data(), riffId, 4);
        info.length = riffChunkSize;
        riffChunks.push_back(info);

        if (fseek(stream, riffChunkSize, SEEK_CUR) != 0)
            return false;
    }

    return true;
}

size_t FileMetadataReader::riffChunkCount() const
{
    return impl_->riffChunks_.size();
}

const RiffChunkInfo* FileMetadataReader::riffChunk(size_t index) const
{
    const std::vector<RiffChunkInfo>& riffChunks = impl_->riffChunks_;
    return (index < riffChunks.size()) ? &riffChunks[index] : nullptr;
}

const RiffChunkInfo* FileMetadataReader::riffChunkById(RiffChunkId id) const
{
    for (const RiffChunkInfo& riff : impl_->riffChunks_) {
        if (riff.id == id)
            return &riff;
    }
    return nullptr;
}

size_t FileMetadataReader::readRiffData(size_t index, void* buffer, size_t count)
{
    const RiffChunkInfo* riff = riffChunk(index);
    if (!riff)
        return 0;

    count = (count < riff->length) ? count : riff->length;

    FILE* stream = impl_->stream_.get();
    if (fseek(stream, riff->fileOffset, SEEK_SET) != 0)
        return 0;

    return fread(buffer, 1, count, stream);
}

bool FileMetadataReader::extractRiffInstrument(SF_INSTRUMENT& ins)
{
    const RiffChunkInfo* riff = riffChunkById(RiffChunkId{'s', 'm', 'p', 'l'});
    if (!riff)
        return 0;

    constexpr uint32_t maxLoops = 16;
    constexpr uint32_t maxChunkSize = 9 * 4 + maxLoops * 6 * 4;

    uint8_t data[maxChunkSize];
    uint32_t length = readRiffData(riff->index, data, sizeof(data));

    auto extractU32 = [&data, length](const uint32_t offset) -> uint32_t {
        const uint8_t* bytes = &data[offset];
        if (bytes + 4 > data + length)
            return 0;
        return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    };

    ins.gain = 1;
    ins.basenote = extractU32(0x14 - 8);
    ins.detune = static_cast<unsigned char>( // Q0,32 semitones to cents
        (static_cast<uint64_t>(extractU32(0x18 - 8)) * 100) >> 32);
    ins.velocity_lo = 0;
    ins.velocity_hi = 127;
    ins.key_lo = 0;
    ins.key_hi = 127;

    const uint32_t numLoops = std::min(maxLoops, extractU32(0x24 - 8));
    ins.loop_count = numLoops;

    for (uint32_t i = 0; i < numLoops; ++i) {
        const uint32_t loopOffset = 0x2c - 8 + i * 24;

        switch (extractU32(loopOffset + 0x04)) {
        default:
            ins.loops[i].mode = SF_LOOP_NONE;
            break;
        case 0:
            ins.loops[i].mode = SF_LOOP_FORWARD;
            break;
        case 1:
            ins.loops[i].mode = SF_LOOP_ALTERNATING;
            break;
        case 2:
            ins.loops[i].mode = SF_LOOP_BACKWARD;
            break;
        }

        ins.loops[i].start = extractU32(loopOffset + 0x08);
        ins.loops[i].end = extractU32(loopOffset + 0x0c) + 1;
        ins.loops[i].count = extractU32(loopOffset + 0x14);
    }

    return true;
}

} // namespace sfz
