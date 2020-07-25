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

class FileMetadataReader {
public:
    FileMetadataReader();
    ~FileMetadataReader();

    bool open(const fs::path& path);
    void close();

    size_t riffChunkCount() const;
    const RiffChunkInfo* riffChunk(size_t index) const;
    const RiffChunkInfo* riffChunkById(RiffChunkId id) const;
    size_t readRiffData(size_t index, void* buffer, size_t count);

    bool extractRiffInstrument(SF_INSTRUMENT& ins);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sfz
