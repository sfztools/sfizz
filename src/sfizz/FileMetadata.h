// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ghc/fs_std.hpp"
#include <sndfile.h>
#include <array>
#include <memory>
#include <cstdio>

namespace sfz {

typedef std::array<char, 4> RiffChunkId;

struct RiffChunkInfo {
    size_t index;
    off_t fileOffset;
    RiffChunkId id;
    uint32_t length;
};

struct WavetableInfo {
    /**
       @brief Size of each successive table in the file
     */
    uint32_t tableSize;
    /**
     * @brief Mode of interpolation between multiple tables
     *
     * 0: none, 1: crossfade, 2: spectral,
     * 3: spectral with fundamental phase set to zero
     * 4: spectral with all phases set to zero
     */
    int crossTableInterpolation;
    /**
     * @brief Whether the wavetable is one-shot (does not cycle)
     */
    bool oneShot;
};

class FileMetadataReader {
public:
    FileMetadataReader();
    ~FileMetadataReader();

    /**
     * @brief Open an audio file of supported format and read internal structures
     */
    bool open(const fs::path& path);
    /**
     * @brief Close an audio file
     */
    void close();

    /**
     * @brief Get the number of RIFF chunks in the file
     */
    size_t riffChunkCount() const;
    /**
     * @brief Get the information regarding the n-th RIFF chunk
     */
    const RiffChunkInfo* riffChunk(size_t index) const;
    /**
     * @brief Get the information regarding the RIFF chunk of given identifier
     */
    const RiffChunkInfo* riffChunkById(RiffChunkId id) const;
    /**
     * @brief Read the RIFF data up to the size given (header not included)
     */
    size_t readRiffData(size_t index, void* buffer, size_t count);

    /**
     * @brief Extract the RIFF 'smpl' data and convert it to sndfile instrument
     */
    bool extractRiffInstrument(SF_INSTRUMENT& ins);

    /**
     * @brief Extract the wavetable information from various relevant RIFF chunks
     */
    bool extractWavetableInfo(WavetableInfo& wt);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sfz
