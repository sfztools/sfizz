// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "FileInstrument.h"
#include "absl/types/span.h"
#include <memory>
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

/**
 * @brief Extract the instrument data from the RIFF sampler block
 *
 * @param data sampler block data, except the 8 leading bytes 'smpl' + size
 * @param ins destination instrument
 */
static bool extractSamplerChunkInstrument(
    absl::Span<const uint8_t> data, SF_INSTRUMENT& ins)
{
    auto extractU32 = [&data](const uint32_t offset) -> uint32_t {
        const uint8_t* bytes = &data[offset];
        if (bytes + 4 > data.end())
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

    const uint32_t numLoops = std::min(16u, extractU32(0x24 - 8));
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

bool FileInstruments::extractFromFlac(const fs::path& path, SF_INSTRUMENT& ins)
{
    memset(&ins, 0, sizeof(SF_INSTRUMENT));

#if !defined(_WIN32)
    FILE_u stream(fopen(path.c_str(), "rb"));
#else
    FILE_u stream(_wfopen(path.wstring().c_str(), L"rb"));
#endif

    char magic[4];
    if (fread(magic, 4, 1, stream.get()) != 1 || memcmp(magic, "fLaC", 4) != 0)
        return false;

    uint32_t header = 0;
    while (((header >> 31) & 1) != 1) {
        if (!fread_u32be(stream.get(), header))
            return false;

        const uint32_t block_type = (header >> 24) & 0x7f;
        const uint32_t block_size = header & ((1 << 24) - 1);

        const off_t off_start_block = ftell(stream.get());
        const off_t off_next_block = off_start_block + block_size;

        if (block_type == 2) { // APPLICATION block
            char blockId[4];
            char riffId[4];
            uint32_t riffChunkSize;
            if (fread(blockId, 4, 1, stream.get()) == 1 && memcmp(blockId, "riff", 4) == 0 &&
                fread(riffId, 4, 1, stream.get()) == 1 && memcmp(riffId, "smpl", 4) == 0 &&
                fread_u32le(stream.get(), riffChunkSize) && riffChunkSize <= block_size - 12)
            {
                std::unique_ptr<uint8_t[]> chunk { new uint8_t[riffChunkSize] };
                if (fread(chunk.get(), riffChunkSize, 1, stream.get()) == 1)
                    return extractSamplerChunkInstrument(
                        { chunk.get(), riffChunkSize }, ins);
            }
        }

        if (fseek(stream.get(), off_next_block, SEEK_SET) != 0)
            return false;
    }

    return false;
}

} // namespace sfz
